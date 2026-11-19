#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "CalibrationOptions.hpp"
#include "RawAdcCsvRecorder.hpp"

namespace
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    int passed{};
    int failed{};
    void check(bool value, const char* label)
    {
        if (value) ++passed;
        else { ++failed; std::cerr << "FAILED: " << label << '\n'; }
    }
    template<typename Function> bool throws(Function action)
    {
        try { action(); } catch (const std::exception&) { return true; }
        return false;
    }

    class TempDirectory
    {
    public:
        TempDirectory()
        {
            const auto stamp = Clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() / ("railflow-calibration-test-" + std::to_string(stamp));
            if (!std::filesystem::create_directory(path)) throw std::runtime_error("Cannot create test directory");
        }
        ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
        std::filesystem::path path;
    };

    class TimedReplay final : public SerialLineTransport
    {
    public:
        TimedReplay(Clock::time_point& clock, std::vector<ReceivedSerialLine> lines)
            : clock_(clock), lines_(std::move(lines)) {}
        std::optional<ReceivedSerialLine> readLine() override
        {
            ++reads;
            if (next_ == lines_.size()) { clock_ += 10ms; return std::nullopt; }
            auto result = lines_[next_++];
            clock_ = std::max(clock_, result.receivedAt);
            return result;
        }
        std::size_t reads{};
    private:
        Clock::time_point& clock_;
        std::vector<ReceivedSerialLine> lines_;
        std::size_t next_{};
    };

    std::string frame(unsigned int sequence, int base)
    {
        std::string line = "RF1," + std::to_string(sequence) + ",123";
        for (int channel = 0; channel < 9; ++channel) line += ',' + std::to_string(base + channel);
        return line;
    }
    std::vector<std::string> rows(const std::string& path)
    {
        std::ifstream file(path);
        std::vector<std::string> result;
        for (std::string line; std::getline(file, line);) result.push_back(line);
        return result;
    }
    using Args = std::vector<std::string_view>;
    Args baseArgs()
    {
        return {"/dev/test", "--vehicle", "CAR1", "--weight-g", "500", "--position", "CENTER",
                "--trial", "1", "--output", "data/calibration/car1.csv"};
    }
}

int main()
{
    try
    {
        TempDirectory directory;
        const CalibrationMetadata metadata{CalibrationVehicle::Car2, 500, CalibrationPosition::Left, 2,
                                           "session,one", "repeat \"left\""};
        auto options = parseCalibrationOptions(baseArgs());
        check(options.metadata.knownWeightG == 500 && options.metadata.trial == 1, "CLI required labels");
        check(options.capture.duration == 5s && options.capture.settleTime == 1s
              && !options.capture.samples && options.baudRate == 115200, "CLI defaults");
        check(parseCalibrationOptions(Args{"--help"}).help, "CLI help");
        auto extended = baseArgs();
        extended.insert(extended.end(), {"--duration-sec", "7", "--settle-ms", "0", "--samples", "50",
            "--baud", "9600", "--session-id", "abc", "--note", "hello, world"});
        options = parseCalibrationOptions(extended);
        check(options.capture.duration == 7s && options.capture.settleTime == 0ms
              && options.capture.samples == 50 && options.metadata.note == "hello, world", "CLI optional labels");
        for (const auto name : {"CAR1", "CAR2", "CAR3"})
            check(parseCalibrationVehicle(name).has_value(), "accepted vehicle");
        for (const auto name : {"CENTER", "LEFT", "RIGHT", "FRONT", "REAR"})
            check(parseCalibrationPosition(name).has_value(), "accepted position");
        check(calibrationFirstChannel(CalibrationVehicle::Car1) == 0
              && calibrationFirstChannel(CalibrationVehicle::Car2) == 3
              && calibrationFirstChannel(CalibrationVehicle::Car3) == 6, "CAR sensor indices");
        for (auto pair : std::vector<std::pair<std::size_t, std::string_view>>{
            {2,"car1"}, {2,"CAR4"}, {4,""}, {4,"-1"}, {4,"-0"}, {4,"NaN"}, {4,"inf"}, {4,"1e999"},
            {4,"500x"}, {6,"center"}, {6,"BACK"}, {8,"0"}, {8,"-1"}, {8,"4294967296"}, {10,""}})
        {
            auto invalid = baseArgs();
            invalid[pair.first] = pair.second;
            check(throws([&] { parseCalibrationOptions(invalid); }), "invalid CLI label rejected");
        }
        for (const auto& extra : {Args{"--duration-sec","0"}, Args{"--duration-sec","3601"},
            Args{"--duration-sec","1.5"}, Args{"--settle-ms","-1"}, Args{"--settle-ms","60001"},
            Args{"--samples","0"}, Args{"--samples","-1"}, Args{"--baud","12345"},
            Args{"--note","line\nbreak"}, Args{"--vehicle","CAR2"}, Args{"--unknown","x"}, Args{"--note"}})
        {
            auto invalid = baseArgs();
            invalid.insert(invalid.end(), extra.begin(), extra.end());
            check(throws([&] { parseCalibrationOptions(invalid); }), "invalid CLI option rejected");
        }
        check(throws([] { parseCalibrationOptions(Args{}); }), "missing device rejected");
        check(throws([] { parseCalibrationOptions(Args{"/dev/test"}); }), "missing required labels rejected");
        for (const auto weight : {"0", "2000", "2500.5"})
        {
            auto args = baseArgs(); args[4] = weight;
            check(parseCalibrationOptions(args).metadata.knownWeightG >= 0, "reference label not capacity-clamped");
        }
        auto badMetadata = metadata;
        badMetadata.knownWeightG = std::numeric_limits<double>::quiet_NaN();
        check(throws([&] { validateCalibrationMetadata(badMetadata); }), "library rejects NaN label");
        badMetadata = metadata; badMetadata.sessionId = std::string(1025, 'x');
        check(throws([&] { validateCalibrationMetadata(badMetadata); }), "metadata length bounded");
        badMetadata = metadata; badMetadata.note = std::string("nul\0note", 8);
        check(throws([&] { validateCalibrationMetadata(badMetadata); }), "embedded NUL rejected");
        check(throws([] { validateCalibrationCaptureConfig({-1ms, 1s, {}}); }), "negative settle rejected");
        check(throws([] { validateCalibrationCaptureConfig({0ms, 0ms, {}}); }), "zero duration rejected");

        const Clock::time_point origin{100s};
        auto clock = origin;
        const auto path = (directory.path / "nested" / "car2.csv").string();
        TimedReplay replay(clock, {
            {frame(10,10), origin+20ms}, {"invalid in settling", origin+99ms},
            {frame(11,20), origin+100ms}, {"RF1,invalid", origin+120ms},
            {frame(14,40), origin+150ms}, {frame(14,60), origin+160ms},
            {frame(12,80), origin+170ms}, {frame(15,100), origin+300ms}
        });
        CalibrationCaptureStats stats;
        {
            CalibrationDatasetRecorder recorder(path, metadata);
            stats = runCalibrationCapture(replay, recorder, {100ms,200ms,{}}, [] { return false; }, [&] { return clock; });
        }
        check(stats.validFrames == 4 && stats.invalidFrames == 1, "valid collected / invalid excluded");
        check(stats.settlingLines == 2 && stats.skippedFrames == 2, "settling excluded with tracker continuity");
        check(stats.duplicateFrames == 1 && stats.backwardFrames == 1, "duplicate/backward flagged, not silently removed");
        check(!stats.interrupted && replay.reads == 8, "duration end boundary excluded");
        check(stats.adc[3].mean == 53 && stats.adc[3].minimum == 23 && stats.adc[3].maximum == 83,
              "selected CAR raw mean/min/max");
        check(std::abs(stats.adc[3].standardDeviation() - std::sqrt(500.0)) < 1e-9, "population standard deviation");
        const auto data = rows(path);
        check(data.size() == 5 && data.front() == CalibrationDatasetRecorder::header(), "CSV header and row count");
        check(data[1] == "100100000000,11,123,CAR2,500,LEFT,2,20,21,22,23,24,25,26,27,28,Sequential,0,\"session,one\",\"repeat \"\"left\"\"\"",
              "exact schema / metadata / nine ADC / timestamp / CSV escaping");
        check(data[2].find(",Skipped,2,") != std::string::npos, "CSV skipped status/count");
        check(CalibrationDatasetRecorder::header() != RawAdcCsvRecorder::header(), "raw CSV schema separate");
        std::ostringstream summary;
        printCalibrationSummary(summary, metadata, stats, path);
        check(summary.str().find("adc3 = mean=53 min=23 max=83") != std::string::npos
              && summary.str().find("adc0 =") == std::string::npos, "summary selects requested CAR only");

        clock = origin;
        TimedReplay samples(clock, {{frame(0,1),origin+1ms}, {frame(1,2),origin+2ms}, {frame(2,3),origin+3ms}});
        {
            CalibrationDatasetRecorder recorder(path, metadata);
            const auto result = runCalibrationCapture(samples, recorder, {0ms,1s,2}, [] { return false; }, [&] { return clock; });
            check(result.validFrames == 2 && samples.reads == 2, "sample count ends before duration");
        }
        check(rows(path).size() == 7 && rows(path).front() == CalibrationDatasetRecorder::header(), "append header once");

        clock = origin;
        TimedReplay idle(clock, {});
        {
            CalibrationDatasetRecorder recorder((directory.path / "empty.csv").string(), metadata);
            const auto result = runCalibrationCapture(idle, recorder, {0ms,30ms,50}, [] { return false; }, [&] { return clock; });
            check(result.validFrames == 0 && idle.reads == 3, "duration caps samples when stream is silent");
            std::ostringstream emptySummary;
            printCalibrationSummary(emptySummary, metadata, result, "empty.csv");
            check(emptySummary.str().find("n/a (no samples)") != std::string::npos, "empty summary does not fabricate stats");
        }
        // Delayed dequeue of a line received during settling must still be excluded.
        clock = origin;
        TimedReplay queued(clock, {{frame(0,1),origin+20ms}, {frame(1,2),origin+110ms}});
        const auto queuedPath = (directory.path / "queued.csv").string();
        {
            CalibrationDatasetRecorder recorder(queuedPath, metadata);
            auto clockCall = [&] { clock = origin+105ms; return origin; };
            // The initial clock call establishes the origin; subsequent calls expose delivery delay.
            bool first = true;
            const auto delayedClock = [&] { if (first) { first=false; return clockCall(); } return clock; };
            const auto result = runCalibrationCapture(queued, recorder, {100ms,100ms,1}, [] { return false; }, delayedClock);
            check(result.settlingLines == 1 && result.validFrames == 1, "receive-time settling, not dequeue time");
        }

        const auto interruptPath = (directory.path / "interrupt.csv").string();
        clock = origin;
        TimedReplay interrupted(clock, {{frame(0,5),origin+1ms}, {frame(1,6),origin+2ms}});
        {
            CalibrationDatasetRecorder recorder(interruptPath, metadata);
            const auto result = runCalibrationCapture(interrupted, recorder, {0ms,1s,{}},
                [&] { return interrupted.reads >= 2; }, [&] { return clock; });
            check(result.interrupted && result.validFrames == 1, "stop after read prevents further writes");
        }
        check(rows(interruptPath).size() == 2, "interrupted CSV flushed and closed");
        { CalibrationDatasetRecorder reopened(interruptPath, metadata); }
        check(rows(interruptPath).size() == 2, "interrupted file safely reopened");
        const auto badPath = (directory.path / "bad.csv").string();
        { std::ofstream bad(badPath); bad << RawAdcCsvRecorder::header() << '\n'; }
        check(throws([&] { CalibrationDatasetRecorder bad(badPath, metadata); }), "incompatible raw header rejected");
        check(rows(badPath).front() == RawAdcCsvRecorder::header(), "incompatible file preserved");
        const auto partialPath = (directory.path / "partial.csv").string();
        { std::ofstream partial(partialPath); partial << CalibrationDatasetRecorder::header() << "\npartial"; }
        check(throws([&] { CalibrationDatasetRecorder bad(partialPath, metadata); }), "incomplete final row rejected");
        const auto zeroPath = (directory.path / "zero.csv").string();
        { std::ofstream empty(zeroPath); }
        { CalibrationDatasetRecorder empty(zeroPath, metadata); }
        check(rows(zeroPath).size() == 1, "empty existing file gets header");
    }
    catch (const std::exception& error)
    {
        ++failed; std::cerr << "Unexpected exception: " << error.what() << '\n';
    }
    std::cout << "Calibration tests: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
