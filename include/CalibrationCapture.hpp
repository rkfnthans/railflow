#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string_view>
#include "CalibrationDataset.hpp"
#include "SerialLineTransport.hpp"

struct CalibrationCaptureConfig
{
    std::chrono::milliseconds settleTime{1000};
    std::chrono::milliseconds duration{5000};
    std::optional<std::size_t> samples; // If set, stop at this count OR duration, whichever comes first.
};

void validateCalibrationCaptureConfig(const CalibrationCaptureConfig& config);

struct CalibrationAdcStats
{
    std::size_t count{};
    double mean{};
    double squaredDeviations{};
    int minimum{FsrProtocol::ADC_MAX};
    int maximum{};
    void observe(int adc);
    double standardDeviation() const; // Population standard deviation, descriptive only.
};

struct CalibrationCaptureStats
{
    std::size_t validFrames{}; // Recorded frames, including flagged Duplicate/Backward.
    std::size_t invalidFrames{}; // Measurement window only.
    std::size_t settlingLines{};
    std::uint64_t skippedFrames{};
    std::size_t duplicateFrames{};
    std::size_t backwardFrames{};
    bool interrupted{};
    std::array<CalibrationAdcStats, FsrProtocol::CHANNEL_COUNT> adc;
};

using CalibrationClock = std::function<std::chrono::steady_clock::time_point()>;

CalibrationCaptureStats runCalibrationCapture(SerialLineTransport& transport,
    CalibrationDatasetRecorder& recorder, const CalibrationCaptureConfig& config,
    const std::function<bool()>& stopRequested,
    const CalibrationClock& now = [] { return std::chrono::steady_clock::now(); });

void printCalibrationSummary(std::ostream& output, const CalibrationMetadata& metadata,
    const CalibrationCaptureStats& stats, std::string_view path);
