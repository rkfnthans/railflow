#include "SerialMonitor.hpp"

#include <ostream>
#include "SerialFrameParser.hpp"
#include "TrainLayout.hpp"

SerialMonitorStats runSerialMonitor(SerialLineTransport& transport, std::ostream& output,
    std::ostream& errors, RawAdcCsvRecorder* recorder, const std::function<bool()>& stopRequested,
    std::size_t maxFrames)
{
    SerialMonitorStats stats;
    SerialFrameTracker tracker;
    while (!stopRequested() && (maxFrames == 0 || stats.validFrames < maxFrames))
    {
        const auto line = transport.readLine();
        if (!line) continue;
        const auto frame = parseFsrSerialFrame(line->text);
        if (!frame)
        {
            errors << "Invalid RF1 line; invalid_count=" << ++stats.invalidLines << '\n';
            continue;
        }
        const auto sequence = tracker.observe(frame->sequence);
        ++stats.validFrames;
        output << "seq=" << frame->sequence << " ms=" << frame->deviceMillis << '\n';
        for (std::size_t car = 0; car < TrainLayout::CAR_COUNT; ++car)
        {
            output << "CAR" << car + 1 << " raw:";
            for (std::size_t sensor = 0; sensor < TrainLayout::FSR_PER_CAR; ++sensor)
                output << ' ' << frame->adc[TrainLayout::channelIndex(car, sensor)];
            output << '\n';
        }
        output << "status=" << serialSequenceStatusName(sequence.status)
               << " skipped=" << sequence.skippedFrames << '\n' << std::flush;
        if (recorder) recorder->record({*frame, line->receivedAt}, sequence);
    }
    return stats;
}
