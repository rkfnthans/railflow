#pragma once

#include <array>
#include <vector>

#include "Passenger.hpp"
#include "PlatformZone.hpp"


bool isInsidePlatformArea(
    const Passenger& passenger);


bool isInsideCoreZone(
    const Passenger& passenger,
    const PlatformZone& zone);


int findCoreZone(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones);


double calculateDistanceToDoor(
    const Passenger& passenger,
    const PlatformZone& zone);


int findNearestExtendedZone(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones);


int classifyPassenger(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones);


int countPassengersInZones(
    const std::vector<Passenger>& passengers,
    std::array<PlatformZone, 3>& zones);