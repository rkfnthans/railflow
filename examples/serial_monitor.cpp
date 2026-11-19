#include <charconv>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include "ConsoleSupport.hpp"
#include "NativeSerialLineTransport.hpp"
#include "SerialMonitor.hpp"

namespace
{
    template<typename T> T number(std::string_view value)
    {
        T parsed{};
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            throw std::invalid_argument("Invalid numeric option");
        return parsed;
    }

    void usage()
    {
        std::cout << "Usage: railflow_serial_monitor <device> [--baud 115200] [--record path]\n"
                     "       [--timeout-ms 100] [--frames N] (0 frames = continuous)\n";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--help") { usage(); return 0; }
        if (argc < 2) { usage(); return 1; }
        SerialConfig config;
        config.stopRequested = consoleStopRequested;
        std::string recordPath;
        std::size_t maxFrames{};
        for (int i = 2; i < argc; ++i)
        {
            const std::string_view option = argv[i];
            if (++i == argc) throw std::invalid_argument("Missing option value");
            const std::string_view value = argv[i];
            if (option == "--baud") config.baudRate = number<unsigned int>(value);
            else if (option == "--record") recordPath = value;
            else if (option == "--frames") maxFrames = number<std::size_t>(value);
            else if (option == "--timeout-ms")
            {
                const auto timeout = number<unsigned int>(value);
                if (timeout == 0 || timeout > 60000) throw std::invalid_argument("Timeout must be 1..60000 ms");
                config.readTimeout = std::chrono::milliseconds{timeout};
            }
            else throw std::invalid_argument("Unknown option: " + std::string(option));
        }
        installConsoleStopHandler();
        NativeSerialLineTransport transport(argv[1], config);
        std::unique_ptr<RawAdcCsvRecorder> recorder;
        if (!recordPath.empty()) recorder = std::make_unique<RawAdcCsvRecorder>(recordPath);
        std::cout << "Serial opened: " << argv[1] << " baud=" << config.baudRate << '\n' << std::flush;
        const auto stats = runSerialMonitor(transport, std::cout, std::cerr, recorder.get(),
            consoleStopRequested, maxFrames);
        std::cout << "valid_frames=" << stats.validFrames << " invalid_lines=" << stats.invalidLines
                  << " overlong_lines=" << transport.discardedOverlongLines() << '\n';
        // RAII also closes both resources on all exception paths.
        transport.close();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Serial monitor: " << error.what() << '\n';
        printCommandLineDiagnostics(argc, argv);
        return 1;
    }
}
