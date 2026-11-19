#pragma once

#include <cstddef>
#include <functional>
#include <iosfwd>
#include "RawAdcCsvRecorder.hpp"
#include "SerialLineTransport.hpp"

struct SerialMonitorStats
{
    std::size_t validFrames{};
    std::size_t invalidLines{};
};

// maxFrames == 0 means continuous monitoring. No weight estimation or core update.
SerialMonitorStats runSerialMonitor(SerialLineTransport& transport, std::ostream& output,
    std::ostream& errors, RawAdcCsvRecorder* recorder, const std::function<bool()>& stopRequested,
    std::size_t maxFrames = 0);
