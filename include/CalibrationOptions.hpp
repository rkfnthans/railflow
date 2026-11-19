#pragma once

#include <span>
#include <string>
#include <string_view>
#include "CalibrationCapture.hpp"

struct CalibrationOptions
{
    bool help{};
    std::string device;
    std::string output;
    unsigned int baudRate{115200};
    CalibrationMetadata metadata;
    CalibrationCaptureConfig capture;
};

// Validate all CLI labels and bounds before opening a port or creating a file.
CalibrationOptions parseCalibrationOptions(std::span<const std::string_view> args);
