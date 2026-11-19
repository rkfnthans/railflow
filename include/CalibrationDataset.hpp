#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include "FsrRawFrame.hpp"
#include "SerialFrameTracker.hpp"

enum class CalibrationVehicle { Car1, Car2, Car3 };
enum class CalibrationPosition { Center, Left, Right, Front, Rear };

std::optional<CalibrationVehicle> parseCalibrationVehicle(std::string_view text);
std::optional<CalibrationPosition> parseCalibrationPosition(std::string_view text);
std::string_view calibrationVehicleName(CalibrationVehicle vehicle);
std::string_view calibrationPositionName(CalibrationPosition position);
std::size_t calibrationFirstChannel(CalibrationVehicle vehicle);

struct CalibrationMetadata
{
    CalibrationVehicle vehicle{CalibrationVehicle::Car1};
    double knownWeightG{}; // User-supplied reference mass label; never inferred from ADC.
    CalibrationPosition position{CalibrationPosition::Center};
    std::uint32_t trial{1};
    std::string sessionId;
    std::string note;
};

void validateCalibrationMetadata(const CalibrationMetadata& metadata);

class CalibrationDatasetRecorder
{
public:
    CalibrationDatasetRecorder(const std::string& path, CalibrationMetadata metadata);
    void record(const ReceivedFsrRawFrame& received, SerialSequenceResult sequence);
    static std::string_view header();
private:
    CalibrationMetadata metadata_;
    std::ofstream file_;
};
