#pragma once

#include <cstddef>
#include <span>
#include <string_view>

enum class CameraBackend { Automatic, DirectShow, MediaFoundation };

struct CameraPreviewOptions
{
    int cameraIndex{0};
    CameraBackend backend{CameraBackend::Automatic};
    int width{640};
    int height{480};
    int framesPerSecond{30};
    std::size_t maxFrames{}; // Zero means continuous.
    bool preview{true};
    bool probe{};
    bool testPattern{};
    bool help{};
};

CameraPreviewOptions parseCameraPreviewOptions(std::span<const std::string_view> arguments);
