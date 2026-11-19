#include "CalibrationDataset.hpp"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>
#include <utility>
#include "RawAdcCsvRecorder.hpp"

std::optional<CalibrationVehicle> parseCalibrationVehicle(std::string_view text)
{
    if (text == "CAR1") return CalibrationVehicle::Car1;
    if (text == "CAR2") return CalibrationVehicle::Car2;
    if (text == "CAR3") return CalibrationVehicle::Car3;
    return std::nullopt;
}

std::optional<CalibrationPosition> parseCalibrationPosition(std::string_view text)
{
    if (text == "CENTER") return CalibrationPosition::Center;
    if (text == "LEFT") return CalibrationPosition::Left;
    if (text == "RIGHT") return CalibrationPosition::Right;
    if (text == "FRONT") return CalibrationPosition::Front;
    if (text == "REAR") return CalibrationPosition::Rear;
    return std::nullopt;
}

std::string_view calibrationVehicleName(CalibrationVehicle vehicle)
{
    switch (vehicle)
    {
    case CalibrationVehicle::Car1: return "CAR1";
    case CalibrationVehicle::Car2: return "CAR2";
    case CalibrationVehicle::Car3: return "CAR3";
    }
    throw std::invalid_argument("Invalid calibration vehicle");
}

std::string_view calibrationPositionName(CalibrationPosition position)
{
    switch (position)
    {
    case CalibrationPosition::Center: return "CENTER";
    case CalibrationPosition::Left: return "LEFT";
    case CalibrationPosition::Right: return "RIGHT";
    case CalibrationPosition::Front: return "FRONT";
    case CalibrationPosition::Rear: return "REAR";
    }
    throw std::invalid_argument("Invalid calibration position");
}

std::size_t calibrationFirstChannel(CalibrationVehicle vehicle)
{
    switch (vehicle)
    {
    case CalibrationVehicle::Car1: return 0;
    case CalibrationVehicle::Car2: return 3;
    case CalibrationVehicle::Car3: return 6;
    }
    throw std::invalid_argument("Invalid calibration vehicle");
}

void validateCalibrationMetadata(const CalibrationMetadata& metadata)
{
    (void)calibrationVehicleName(metadata.vehicle);
    (void)calibrationPositionName(metadata.position);
    if (!std::isfinite(metadata.knownWeightG) || metadata.knownWeightG < 0 || std::signbit(metadata.knownWeightG))
        throw std::invalid_argument("Known weight must be finite and nonnegative");
    if (metadata.trial == 0) throw std::invalid_argument("Trial must be positive");
    for (const auto* text : {&metadata.sessionId, &metadata.note})
    {
        if (text->size() > 1024) throw std::invalid_argument("Metadata text exceeds 1024 bytes");
        for (unsigned char ch : *text)
            if (ch < 32 || ch == 127) throw std::invalid_argument("Metadata text cannot contain control characters");
    }
}

namespace
{
    std::string csvQuoted(const std::string& text)
    {
        std::string result = "\"";
        for (char ch : text)
        {
            if (ch == '"') result += '"';
            result += ch;
        }
        return result + '"';
    }
}

std::string_view CalibrationDatasetRecorder::header()
{
    return "host_time_steady_ns,sequence,deviceMillis,vehicle_id,known_weight_g,load_position,trial,adc0,adc1,adc2,adc3,adc4,adc5,adc6,adc7,adc8,tracker_status,skipped_frames,session_id,note";
}

CalibrationDatasetRecorder::CalibrationDatasetRecorder(const std::string& path, CalibrationMetadata metadata)
    : metadata_(std::move(metadata))
{
    validateCalibrationMetadata(metadata_);
    if (path.empty()) throw std::invalid_argument("Dataset output path is empty");
    const bool hasContent = std::filesystem::exists(path) && std::filesystem::file_size(path) != 0;
    if (hasContent)
    {
        std::ifstream existing(path, std::ios::binary);
        std::string firstLine;
        if (!std::getline(existing, firstLine)) throw std::runtime_error("Cannot read calibration CSV header");
        if (!firstLine.empty() && firstLine.back() == '\r') firstLine.pop_back();
        if (firstLine != header()) throw std::runtime_error("Incompatible calibration CSV header");
        existing.clear();
        existing.seekg(-1, std::ios::end);
        if (existing.get() != '\n') throw std::runtime_error("Calibration CSV has an incomplete final row");
    }
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    file_.open(path, std::ios::app);
    if (!file_) throw std::runtime_error("Cannot open calibration CSV: " + path);
    file_.imbue(std::locale::classic());
    file_ << std::setprecision(std::numeric_limits<double>::max_digits10);
    if (!hasContent)
    {
        file_ << header() << '\n';
        file_.flush();
        if (!file_) throw std::runtime_error("Cannot write calibration CSV header");
    }
}

void CalibrationDatasetRecorder::record(const ReceivedFsrRawFrame& received, SerialSequenceResult sequence)
{
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(received.receivedAt.time_since_epoch()).count();
    file_ << ns << ',' << received.frame.sequence << ',' << received.frame.deviceMillis << ','
          << calibrationVehicleName(metadata_.vehicle) << ',' << metadata_.knownWeightG << ','
          << calibrationPositionName(metadata_.position) << ',' << metadata_.trial;
    for (int adc : received.frame.adc) file_ << ',' << adc;
    file_ << ',' << serialSequenceStatusName(sequence.status) << ',' << sequence.skippedFrames
          << ',' << csvQuoted(metadata_.sessionId) << ',' << csvQuoted(metadata_.note) << '\n';
    file_.flush();
    if (!file_) throw std::runtime_error("Cannot write calibration CSV row");
}
