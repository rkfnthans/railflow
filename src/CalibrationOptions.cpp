#include "CalibrationOptions.hpp"

#include <charconv>
#include <locale>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{
    template<typename T> T numeric(std::string_view value)
    {
        if (value.empty()) throw std::invalid_argument("Empty numeric calibration option");
        T result{};
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
            throw std::invalid_argument("Invalid numeric calibration option");
        return result;
    }

    // Floating-point from_chars requires a much newer macOS libc++ than the
    // rest of RailFlow. Classic-locale extraction keeps the CLI portable.
    template<> double numeric<double>(std::string_view value)
    {
        if (value.empty()) throw std::invalid_argument("Empty numeric calibration option");
        if (value.front() == '+' || value.find_first_of("xX") != std::string_view::npos)
            throw std::invalid_argument("Invalid numeric calibration option");
        std::istringstream input{std::string(value)};
        input.imbue(std::locale::classic());
        double result{};
        input >> std::noskipws >> result;
        if (!input || input.peek() != std::char_traits<char>::eof())
            throw std::invalid_argument("Invalid numeric calibration option");
        const auto mantissa = value.substr(0, value.find_first_of("eE"));
        if (result == 0.0 && mantissa.find_first_of("123456789") != std::string_view::npos)
            throw std::invalid_argument("Numeric calibration option underflows double precision");
        return result;
    }
}

CalibrationOptions parseCalibrationOptions(std::span<const std::string_view> args)
{
    CalibrationOptions options;
    if (args.size() == 1 && args.front() == "--help") { options.help = true; return options; }
    if (args.empty() || args.front().empty() || args.front().starts_with("--"))
        throw std::invalid_argument("Serial device path is required");
    options.device = args.front();
    std::set<std::string_view> seen;
    for (std::size_t i = 1; i < args.size(); ++i)
    {
        const auto option = args[i];
        if (!seen.insert(option).second) throw std::invalid_argument("Duplicate option: " + std::string(option));
        if (++i == args.size()) throw std::invalid_argument("Missing option value");
        const auto value = args[i];
        if (option == "--vehicle")
        {
            const auto vehicle = parseCalibrationVehicle(value);
            if (!vehicle) throw std::invalid_argument("Vehicle must be CAR1, CAR2 or CAR3 (uppercase)");
            options.metadata.vehicle = *vehicle;
        }
        else if (option == "--position")
        {
            const auto position = parseCalibrationPosition(value);
            if (!position) throw std::invalid_argument("Position must be CENTER, LEFT, RIGHT, FRONT or REAR (uppercase)");
            options.metadata.position = *position;
        }
        else if (option == "--weight-g") options.metadata.knownWeightG = numeric<double>(value);
        else if (option == "--trial") options.metadata.trial = numeric<std::uint32_t>(value);
        else if (option == "--duration-sec")
        {
            const auto seconds = numeric<unsigned int>(value);
            if (seconds == 0 || seconds > 3600) throw std::invalid_argument("Duration must be 1..3600 whole seconds");
            options.capture.duration = std::chrono::seconds{seconds};
        }
        else if (option == "--settle-ms")
        {
            const auto milliseconds = numeric<unsigned int>(value);
            if (milliseconds > 60000) throw std::invalid_argument("Settle time must be 0..60000 ms");
            options.capture.settleTime = std::chrono::milliseconds{milliseconds};
        }
        else if (option == "--samples") options.capture.samples = numeric<std::size_t>(value);
        else if (option == "--baud") options.baudRate = numeric<unsigned int>(value);
        else if (option == "--output") options.output = value;
        else if (option == "--session-id") options.metadata.sessionId = value;
        else if (option == "--note") options.metadata.note = value;
        else throw std::invalid_argument("Unknown calibration option: " + std::string(option));
    }
    for (const auto required : {"--vehicle", "--weight-g", "--position", "--trial", "--output"})
        if (!seen.contains(required)) throw std::invalid_argument("Required option: " + std::string(required));
    if (options.output.empty()) throw std::invalid_argument("Dataset output path is empty");
    switch (options.baudRate)
    {
    case 9600: case 19200: case 38400: case 57600: case 115200: break;
    default: throw std::invalid_argument("Unsupported serial baud rate");
    }
    validateCalibrationMetadata(options.metadata);
    validateCalibrationCaptureConfig(options.capture);
    return options;
}
