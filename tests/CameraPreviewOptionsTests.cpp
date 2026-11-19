#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include "CameraPreviewOptions.hpp"

int main()
{
    const auto parse = [](std::initializer_list<std::string_view> arguments)
    {
        return parseCameraPreviewOptions({arguments.begin(), arguments.size()});
    };
    const auto rejects = [&](std::initializer_list<std::string_view> arguments)
    {
        try { (void)parse(arguments); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    const auto defaults = parse({});
    const auto configured = parse({"--camera", "1", "--backend", "dshow", "--width", "1280",
        "--height", "720", "--fps", "60", "--frames", "30", "--no-preview"});
    const bool passed = defaults.cameraIndex == 0 && defaults.width == 640 && defaults.height == 480
        && defaults.framesPerSecond == 30 && defaults.preview && defaults.maxFrames == 0
        && configured.cameraIndex == 1 && configured.backend == CameraBackend::DirectShow
        && configured.width == 1280 && configured.height == 720 && configured.framesPerSecond == 60
        && configured.maxFrames == 30 && !configured.preview
        && parse({"--backend", "msmf"}).backend == CameraBackend::MediaFoundation
        && parse({"--probe"}).probe && parse({"--test-pattern", "--frames", "3"}).testPattern
        && parse({"--help"}).help
        && rejects({"--camera", "-1"}) && rejects({"--camera", "32"})
        && rejects({"--frames", "3x"}) && rejects({"--frames", "1000001"})
        && rejects({"--width", "0"}) && rejects({"--height", "50000"})
        && rejects({"--fps", "0"}) && rejects({"--backend", "other"})
        && rejects({"--camera"}) && rejects({"--camera", "0", "--camera", "1"})
        && rejects({"--test-pattern", "--camera", "0"})
        && rejects({"--test-pattern", "--probe"}) && rejects({"--probe", "--frames", "3"})
        && rejects({"--unknown"});
    std::cout << (passed ? "Camera preview options passed\n" : "Camera preview options failed\n");
    return passed ? 0 : 1;
}
