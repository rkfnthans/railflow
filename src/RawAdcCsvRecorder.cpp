#include "RawAdcCsvRecorder.hpp"

#include <filesystem>
#include <stdexcept>

std::string_view serialSequenceStatusName(SerialSequenceStatus status)
{
    switch (status)
    {
    case SerialSequenceStatus::First: return "First";
    case SerialSequenceStatus::Sequential: return "Sequential";
    case SerialSequenceStatus::Duplicate: return "Duplicate";
    case SerialSequenceStatus::Skipped: return "Skipped";
    case SerialSequenceStatus::Backward: return "Backward";
    }
    throw std::logic_error("Unknown serial sequence status");
}

std::string_view RawAdcCsvRecorder::header()
{
    return "host_time_steady_ns,sequence,deviceMillis,adc0,adc1,adc2,adc3,adc4,adc5,adc6,adc7,adc8,tracker_status,skipped_frames";
}

RawAdcCsvRecorder::RawAdcCsvRecorder(const std::string& path)
{
    const bool hasContent = std::filesystem::exists(path) && std::filesystem::file_size(path) != 0;
    if (hasContent)
    {
        std::ifstream existing(path, std::ios::binary);
        std::string firstLine;
        if (!std::getline(existing, firstLine)) throw std::runtime_error("Cannot read raw ADC CSV header");
        if (!firstLine.empty() && firstLine.back() == '\r') firstLine.pop_back();
        if (firstLine != header()) throw std::runtime_error("Incompatible raw ADC CSV header");
        existing.clear();
        existing.seekg(-1, std::ios::end);
        if (existing.get() != '\n') throw std::runtime_error("Raw ADC CSV has an incomplete final row");
    }
    file_.open(path, std::ios::app);
    if (!file_) throw std::runtime_error("Cannot open raw ADC CSV: " + path);
    if (!hasContent)
    {
        file_ << header() << '\n';
        file_.flush();
        if (!file_) throw std::runtime_error("Cannot write raw ADC CSV header");
    }
}

void RawAdcCsvRecorder::record(const ReceivedFsrRawFrame& received, SerialSequenceResult sequence)
{
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(received.receivedAt.time_since_epoch()).count();
    file_ << ns << ',' << received.frame.sequence << ',' << received.frame.deviceMillis;
    for (int adc : received.frame.adc) file_ << ',' << adc;
    file_ << ',' << serialSequenceStatusName(sequence.status) << ',' << sequence.skippedFrames << '\n';
    file_.flush();
    if (!file_) throw std::runtime_error("Cannot write raw ADC CSV row");
}
