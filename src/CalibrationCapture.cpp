#include "CalibrationCapture.hpp"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include "SerialFrameParser.hpp"

void validateCalibrationCaptureConfig(const CalibrationCaptureConfig& config)
{
    if (config.settleTime.count() < 0 || config.settleTime.count() > 60000)
        throw std::invalid_argument("Settle time must be 0..60000 ms");
    if (config.duration.count() <= 0 || config.duration.count() > 3600000)
        throw std::invalid_argument("Duration must be positive and at most 3600 seconds");
    if (config.samples && *config.samples == 0) throw std::invalid_argument("Samples must be positive");
}

void CalibrationAdcStats::observe(int adc)
{
    ++count;
    minimum = std::min(minimum, adc);
    maximum = std::max(maximum, adc);
    const double delta = adc - mean;
    mean += delta / static_cast<double>(count);
    squaredDeviations += delta * (adc - mean);
}

double CalibrationAdcStats::standardDeviation() const
{
    return count ? std::sqrt(std::max(0.0, squaredDeviations / static_cast<double>(count))) : 0.0;
}

CalibrationCaptureStats runCalibrationCapture(SerialLineTransport& transport,
    CalibrationDatasetRecorder& recorder, const CalibrationCaptureConfig& config,
    const std::function<bool()>& stopRequested, const CalibrationClock& now)
{
    validateCalibrationCaptureConfig(config);
    const auto recordStart = now() + config.settleTime;
    const auto recordEnd = recordStart + config.duration;
    CalibrationCaptureStats stats;
    SerialFrameTracker tracker;
    while (true)
    {
        if (stopRequested()) { stats.interrupted = true; break; }
        if (now() >= recordEnd || (config.samples && stats.validFrames >= *config.samples)) break;
        const auto line = transport.readLine(); // Caller supplies bounded reads (CLI: 100 ms).
        if (stopRequested()) { stats.interrupted = true; break; }
        if (!line) continue;
        // Use the receive timestamp, not dequeue time, to exclude queued settling frames.
        if (line->receivedAt >= recordEnd) break;
        const auto frame = parseFsrSerialFrame(line->text);
        if (line->receivedAt < recordStart)
        {
            ++stats.settlingLines;
            if (frame) (void)tracker.observe(frame->sequence);
            continue;
        }
        if (!frame) { ++stats.invalidFrames; continue; }
        const auto sequence = tracker.observe(frame->sequence);
        recorder.record({*frame, line->receivedAt}, sequence);
        ++stats.validFrames;
        stats.skippedFrames += sequence.skippedFrames;
        if (sequence.status == SerialSequenceStatus::Duplicate) ++stats.duplicateFrames;
        if (sequence.status == SerialSequenceStatus::Backward) ++stats.backwardFrames;
        for (std::size_t channel = 0; channel < frame->adc.size(); ++channel)
            stats.adc[channel].observe(frame->adc[channel]);
    }
    return stats;
}

void printCalibrationSummary(std::ostream& output, const CalibrationMetadata& metadata,
    const CalibrationCaptureStats& stats, std::string_view path)
{
    output << "Vehicle: " << calibrationVehicleName(metadata.vehicle)
           << "\nKnown weight: " << metadata.knownWeightG << " g (reference label only)"
           << "\nPosition: " << calibrationPositionName(metadata.position)
           << "\nTrial: " << metadata.trial
           << "\nSession: " << metadata.sessionId
           << "\nValid frames: " << stats.validFrames
           << "\nInvalid frames: " << stats.invalidFrames
           << "\nSkipped frames: " << stats.skippedFrames
           << "\nDuplicate frames: " << stats.duplicateFrames
           << "\nBackward frames: " << stats.backwardFrames
           << "\nSettling lines discarded: " << stats.settlingLines
           << "\nInterrupted: " << (stats.interrupted ? "yes" : "no") << '\n';
    const auto first = calibrationFirstChannel(metadata.vehicle);
    output << calibrationVehicleName(metadata.vehicle) << " ADC statistics (raw only):\n";
    for (std::size_t channel = first; channel < first + 3; ++channel)
    {
        const auto& sensor = stats.adc[channel];
        output << "adc" << channel << " = ";
        if (sensor.count == 0) output << "n/a (no samples)\n";
        else output << "mean=" << sensor.mean << " min=" << sensor.minimum << " max=" << sensor.maximum
                    << " stddev=" << sensor.standardDeviation() << '\n';
    }
    output << "Dataset saved: " << path << '\n';
    if (stats.validFrames == 0) output << "WARNING: no frames recorded; check port, stream and settling time.\n";
}
