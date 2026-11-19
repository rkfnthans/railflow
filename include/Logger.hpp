#pragma once

#include <array>
#include <cstddef>

#include "PlatformZone.hpp"
#include "Vehicle.hpp"


void writeLog(
    const std::array<Vehicle, 3>& vehicles,
    const std::array<PlatformZone, 3>& zones,
    std::size_t bestZoneIndex,
    int unassignedPassengers);