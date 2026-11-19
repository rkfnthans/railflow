#pragma once

#include <array>
#include <cstddef>
#include "InputSource.hpp"
#include "PlatformZone.hpp"

struct RailFlowState
{
    InputFrame input;
    std::array<PlatformZone, 3> zones;
    int unassignedPassengers{};
    std::size_t bestZoneIndex{};
};

// 각 호출은 새 입력과 독립적인 Zone 상태를 계산한다.
RailFlowState updateRailFlow(InputFrame input, std::array<PlatformZone, 3> zones);
