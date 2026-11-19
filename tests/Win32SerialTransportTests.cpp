#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>
#include "CalibrationCapture.hpp"
#include "SerialMonitor.hpp"
#include "Win32SerialLineTransport.hpp"
#include "Win32SerialPort.hpp"

namespace
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    enum class ReadEvent { Data, Timeout, Corruption, Disconnected, Interrupt };
    struct ReadStep
    {
        ReadEvent event;
        std::string bytes;
    };

    std::deque<ReadStep> script;
    std::wstring openedPath;
    unsigned int openedBaud{};
    int openCount{};
    int closeCount{};
    bool interrupted{};
    std::chrono::milliseconds longestWait{};

    class FakeComPort final : public Win32SerialPort
    {
    public:
        ~FakeComPort() override { ++closeCount; }

        Win32SerialRead read(std::span<char> buffer, std::chrono::milliseconds timeout) override
        {
            longestWait = std::max(longestWait, timeout);
            ReadStep step{ReadEvent::Timeout, {}};
            if (!script.empty())
            {
                step = std::move(script.front());
                script.pop_front();
            }
            switch (step.event)
            {
            case ReadEvent::Data:
                if (step.bytes.size() > buffer.size()) throw std::logic_error("Fake COM chunk too large");
                std::ranges::copy(step.bytes, buffer.begin());
                return {step.bytes.size(), false};
            case ReadEvent::Timeout:
                std::this_thread::sleep_for(timeout);
                return {};
            case ReadEvent::Corruption:
                return {0, true};
            case ReadEvent::Disconnected:
                throw std::system_error(std::make_error_code(std::errc::no_such_device), "COM disconnected");
            case ReadEvent::Interrupt:
                interrupted = true;
                return {};
            }
            throw std::logic_error("Unknown fake COM event");
        }
    };

    void check(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    template<typename Exception, typename Action>
    void expectThrows(Action action, const char* message)
    {
        try { action(); }
        catch (const Exception&) { return; }
        throw std::runtime_error(message);
    }

    class TempDirectory
    {
    public:
        TempDirectory()
            : path(std::filesystem::temp_directory_path() /
                ("railflow-win32-serial-" + std::to_string(Clock::now().time_since_epoch().count())))
        {
            check(std::filesystem::create_directory(path), "Could not create test directory");
        }
        ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
        const std::filesystem::path path;
    };

    const std::string firstFrame = "RF1,10,50,1,2,3,4,5,6,7,8,9";
    const std::string secondFrame = "RF1,11,100,9,8,7,6,5,4,3,2,1";

    void testPartialAndTimeout()
    {
        Win32SerialLineTransport transport("COM3", {115200, 0ms, 64});
        check(openedPath == LR"(\\.\COM3)" && openedBaud == 115200, "COM3 and RF1 baud passed to driver");
        script = {{ReadEvent::Data, firstFrame.substr(0, 12)}, {ReadEvent::Timeout, {}},
                  {ReadEvent::Data, firstFrame.substr(12) + "\r\n" + secondFrame + "\n"}};
        check(!transport.readLine() && !transport.readLine(), "Partial frame survives an idle timeout");
        const auto before = Clock::now();
        const auto first = transport.readLine();
        const auto second = transport.readLine();
        check(first && first->text == firstFrame && first->receivedAt >= before, "CRLF frame joined and timestamped");
        check(second && second->text == secondFrame && second->receivedAt == first->receivedAt,
            "Second complete line retains the receive timestamp");

        Win32SerialLineTransport idle("COM4", {115200, 20ms, 64});
        const auto start = Clock::now();
        check(!idle.readLine() && Clock::now() - start >= 20ms && idle.isOpen(), "Idle timeout keeps COM4 open");
        check(openedPath == LR"(\\.\COM4)", "COM4 device path");
    }

    void testMonitor(const std::filesystem::path& file)
    {
        script = {{ReadEvent::Data, "RF1,9,12,1024,2,3,4,5,6,7,8,9\n" + std::string(65, 'x')},
                  {ReadEvent::Data, "ignored\n" + firstFrame.substr(0, 12)},
                  {ReadEvent::Corruption, {}},
                  {ReadEvent::Data, firstFrame.substr(12) + "\n" + secondFrame + "\r\n"}};
        Win32SerialLineTransport transport("com10", {115200, 100ms, 64});
        check(openedPath == LR"(\\.\COM10)", "COM10 canonical device path");
        std::ostringstream out;
        std::ostringstream errors;
        {
            RawAdcCsvRecorder csv(file.string());
            const auto stats = runSerialMonitor(transport, out, errors, &csv, [] { return false; }, 1);
            check(stats.validFrames == 1 && stats.invalidLines == 1 && transport.discardedOverlongLines() == 1,
                "Monitor accepts RF1 and rejects out-of-range ADC, overlong and damaged input");
        }
        check(out.str().find("CAR1 raw: 9 8 7\nCAR2 raw: 6 5 4\nCAR3 raw: 3 2 1\n") != std::string::npos,
            "Monitor groups nine ADC values in wire order");
        std::ifstream csv(file);
        std::string header;
        std::string row;
        check(std::getline(csv, header) && header == RawAdcCsvRecorder::header()
            && std::getline(csv, row) && row.ends_with(",11,100,9,8,7,6,5,4,3,2,1,First,0")
            && !std::getline(csv, row), "Monitor CSV has exactly one valid row");
    }

    void testReconnectAndCalibration(const std::filesystem::path& file)
    {
        const int initialCloses = closeCount;
        script = {{ReadEvent::Data, firstFrame.substr(0, 12)}, {ReadEvent::Disconnected, {}}};
        Win32SerialLineTransport lost("COM10");
        expectThrows<std::system_error>([&] { (void)lost.readLine(); }, "Disconnection must report an I/O error");
        check(!lost.isOpen() && closeCount == initialCloses + 1, "Disconnection releases the port immediately");
        expectThrows<std::logic_error>([&] { (void)lost.readLine(); }, "Closed transport cannot be read");

        script = {{ReadEvent::Data, firstFrame.substr(12) + "\n" + secondFrame.substr(0, 10)},
                  {ReadEvent::Data, secondFrame.substr(10) + "\n"}};
        Win32SerialLineTransport reopened(R"(\\.\COM10)");
        check(openedPath == LR"(\\.\COM10)", "COM device can be reopened after a lost connection");
        const CalibrationMetadata metadata{CalibrationVehicle::Car3, 500, CalibrationPosition::Center, 1, "", ""};
        {
            CalibrationDatasetRecorder csv(file.string(), metadata);
            const auto stats = runCalibrationCapture(reopened, csv, {0ms, 2s, 1}, [] { return false; });
            check(stats.validFrames == 1 && stats.invalidFrames == 1 && stats.adc[6].mean == 3
                && stats.adc[8].mean == 1, "New capture excludes stale bytes and observes CAR3 ADC values");
        }
        std::ifstream csv(file);
        std::string header;
        std::string row;
        check(std::getline(csv, header) && header == CalibrationDatasetRecorder::header()
            && std::getline(csv, row) && row.find(",CAR3,500,CENTER,1,9,8,7,6,5,4,3,2,1,First,0,") != std::string::npos
            && !std::getline(csv, row), "Calibration CSV preserves the reference weight and nine raw ADC values");
    }

    void testInvalidPortAndInterrupt()
    {
        const int initialOpens = openCount;
        for (const std::string device : {"COM", "COM0", "COM3:extra", "NUL", "COM4294967296"})
            expectThrows<std::invalid_argument>([&] { Win32SerialLineTransport transport(device); }, "Invalid COM name");
        check(openCount == initialOpens, "Invalid names are rejected before reaching the driver");

        longestWait = 0ms;
        script = {{ReadEvent::Interrupt, {}}};
        Win32SerialLineTransport transport("COM10", {115200, 60s, 512, []() noexcept { return interrupted; }});
        check(!transport.readLine() && interrupted && longestWait == 100ms, "Stop request interrupts a long timeout");
    }
}

std::unique_ptr<Win32SerialPort> openWin32SerialPort(const std::wstring& path, unsigned int baudRate)
{
    ++openCount;
    openedPath = path;
    openedBaud = baudRate;
    return std::make_unique<FakeComPort>();
}

int main()
{
    try
    {
        const TempDirectory directory;
        testPartialAndTimeout();
        testMonitor(directory.path / "monitor.csv");
        testReconnectAndCalibration(directory.path / "calibration.csv");
        testInvalidPortAndInterrupt();
        std::cout << "Win32 serial transport tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Win32 serial transport test failed: " << error.what() << '\n';
        return 1;
    }
}
