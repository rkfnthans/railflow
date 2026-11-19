#pragma once

#include <array>

#include "PlatformZone.hpp"

namespace ZoneConfig
{
    // RailFlow canonical 좌표계이며 실제 카메라 해상도를 의미하지 않는다.
    inline constexpr int FRAME_WIDTH = 1200;
    inline constexpr int FRAME_HEIGHT = 600;

    inline constexpr int PLATFORM_X_MIN = 0;
    inline constexpr int PLATFORM_X_MAX = FRAME_WIDTH;
    inline constexpr int PLATFORM_Y_MIN = 220;
    inline constexpr int PLATFORM_Y_MAX = FRAME_HEIGHT;

    inline constexpr double AMBIGUOUS_DISTANCE_MARGIN = 30.0;

    std::array<PlatformZone, 3> createDefaultZones();
}
