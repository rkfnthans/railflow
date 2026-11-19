#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include "FsrRawFrame.hpp"
#include "SerialFrameTracker.hpp"

std::string_view serialSequenceStatusName(SerialSequenceStatus status);

class RawAdcCsvRecorder
{
public:
    explicit RawAdcCsvRecorder(const std::string& path);
    void record(const ReceivedFsrRawFrame& received, SerialSequenceResult sequence);
    static std::string_view header();
private:
    std::ofstream file_;
};
