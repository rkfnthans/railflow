#include "CameraPreviewOptions.hpp"

#include <charconv>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
    int number(std::string_view text, int minimum, int maximum)
    {
        int result{};
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
            || result < minimum || result > maximum)
            throw std::invalid_argument("Numeric camera option is outside its allowed range");
        return result;
    }
}

CameraPreviewOptions parseCameraPreviewOptions(std::span<const std::string_view> arguments)
{
    CameraPreviewOptions options;
    std::set<std::string_view> seen;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        const auto option = arguments[index];
        if (!seen.insert(option).second) throw std::invalid_argument("Duplicate option: " + std::string(option));
        if (option == "--help") options.help = true;
        else if (option == "--probe") options.probe = true;
        else if (option == "--test-pattern") options.testPattern = true;
        else if (option == "--no-preview") options.preview = false;
        else
        {
            if (option != "--camera" && option != "--backend" && option != "--width"
                && option != "--height" && option != "--fps" && option != "--frames")
                throw std::invalid_argument("Unknown camera option: " + std::string(option));
            if (++index == arguments.size()) throw std::invalid_argument("Missing option value: " + std::string(option));
            const auto value = arguments[index];
            if (option == "--camera") options.cameraIndex = number(value, 0, 31);
            else if (option == "--width") options.width = number(value, 160, 7680);
            else if (option == "--height") options.height = number(value, 120, 4320);
            else if (option == "--fps") options.framesPerSecond = number(value, 1, 240);
            else if (option == "--frames") options.maxFrames = static_cast<std::size_t>(number(value, 0, 1000000));
            else if (value == "auto") options.backend = CameraBackend::Automatic;
            else if (value == "dshow") options.backend = CameraBackend::DirectShow;
            else if (value == "msmf") options.backend = CameraBackend::MediaFoundation;
            else throw std::invalid_argument("Backend must be auto, dshow or msmf");
        }
    }
    if (options.testPattern && (options.probe || seen.contains("--camera") || seen.contains("--backend")))
        throw std::invalid_argument("Test pattern must be used with display settings only");
    if (options.probe && (seen.contains("--camera") || seen.contains("--frames")))
        throw std::invalid_argument("Probe checks camera indices 0..3; omit --camera and --frames");
    return options;
}
