#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "SerialMonitor.hpp"

namespace
{
    class Replay final : public SerialLineTransport
    {
    public:
        explicit Replay(std::vector<ReceivedSerialLine> lines) : lines_(std::move(lines)) {}
        std::optional<ReceivedSerialLine> readLine() override
        {
            if (next_ == lines_.size()) return std::nullopt;
            return lines_[next_++];
        }
        bool exhausted() const { return next_ == lines_.size(); }
    private:
        std::vector<ReceivedSerialLine> lines_;
        std::size_t next_{};
    };

    class TempDirectory
    {
    public:
        TempDirectory()
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() / ("railflow-raw-csv-test-" + std::to_string(stamp));
            if (!std::filesystem::create_directory(path)) throw std::runtime_error("Cannot create test directory");
        }
        ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
        std::filesystem::path path;
    };
    int passed{};
    int failed{};
    void check(bool ok, const char* message)
    {
        if (ok) ++passed;
        else { ++failed; std::cerr << "FAILED: " << message << '\n'; }
    }
    template<typename Function> bool throws(Function action)
    {
        try { action(); } catch (const std::exception&) { return true; }
        return false;
    }
}

int main()
{
    try
    {
        TempDirectory directory;
        const auto path = (directory.path / "raw.csv").string();
        const auto timestamp = std::chrono::steady_clock::time_point{std::chrono::nanoseconds{123456789}};
        Replay replay{{
            {"invalid RF1", timestamp},
            {"RF1,10,100,231,228,235,410,398,405,120,118,121", timestamp},
            {"RF1,11,101,1024,2,3,4,5,6,7,8,9", timestamp},
            {"RF1,11,101,1,2,3,4,5,6,7,8,9", timestamp},
            {"RF1,11,102,1,2,3,4,5,6,7,8,9", timestamp},
            {"RF1,14,103,1,2,3,4,5,6,7,8,9", timestamp},
            {"RF1,12,104,1,2,3,4,5,6,7,8,9", timestamp}
        }};
        std::ostringstream output;
        std::ostringstream errors;
        {
            RawAdcCsvRecorder csv(path);
            const auto stats = runSerialMonitor(replay, output, errors, &csv, [] { return false; }, 5);
            check(stats.validFrames == 5 && stats.invalidLines == 2, "invalid counted and monitoring continues");
        }
        check(replay.exhausted(), "all replay lines processed");
        check(output.str().find("CAR1 raw: 231 228 235\nCAR2 raw: 410 398 405\nCAR3 raw: 120 118 121")
              != std::string::npos, "CAR channel mapping");
        for (const char* status : {"First", "Sequential", "Duplicate", "Skipped", "Backward"})
            check(output.str().find(std::string("status=") + status) != std::string::npos, status);
        check(errors.str().find("invalid_count=2") != std::string::npos, "invalid count reported");
        {
            RawAdcCsvRecorder append(path);
            FsrRawFrame frame{15, 105, {1,2,3,4,5,6,7,8,9}};
            append.record({frame, timestamp}, {SerialSequenceStatus::Sequential, 0});
        }
        std::ifstream file(path);
        std::vector<std::string> rows;
        for (std::string row; std::getline(file, row);) rows.push_back(row);
        check(rows.size() == 7 && rows.front() == RawAdcCsvRecorder::header(), "append and header once");
        check(rows.size() > 1 && rows[1] == "123456789,10,100,231,228,235,410,398,405,120,118,121,First,0",
              "exact raw schema, ADC order and preserved receive timestamp");
        check(rows.size() > 4 && rows[4].ends_with(",Skipped,2"), "skipped count recorded");
        check(RawAdcCsvRecorder::header().find("weight") == std::string_view::npos, "no weight column");

        const auto badPath = (directory.path / "incompatible.csv").string();
        { std::ofstream bad(badPath); bad << "wrong,header\n"; }
        check(throws([&] { RawAdcCsvRecorder bad(badPath); }), "incompatible header not overwritten");
        const auto partialPath = (directory.path / "partial.csv").string();
        { std::ofstream partial(partialPath); partial << RawAdcCsvRecorder::header() << "\n123"; }
        check(throws([&] { RawAdcCsvRecorder partial(partialPath); }), "incomplete final row not appended");
        check(throws([&] { RawAdcCsvRecorder bad((directory.path / "missing" / "raw.csv").string()); }),
              "output open failure");
        Replay idle{{}};
        const auto stopped = runSerialMonitor(idle, output, errors, nullptr, [] { return true; });
        check(stopped.validFrames == 0 && stopped.invalidLines == 0, "stop requested exits immediately");
        Replay finalFrame{{{"RF1,0,0,0,0,0,0,0,0,0,0,0", timestamp}}};
        const auto withoutCsv = runSerialMonitor(finalFrame, output, errors, nullptr,
            [&] { return finalFrame.exhausted(); });
        check(withoutCsv.validFrames == 1, "monitor without recording and callback stop");
    }
    catch (const std::exception& error)
    {
        ++failed;
        std::cerr << "Unexpected exception: " << error.what() << '\n';
    }
    std::cout << "SerialMonitor tests: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
