#include "ZoneManager.hpp"
#include "ZoneConfig.hpp"

#include <cmath>
#include <limits>


// ============================================================
// 전체 승강장 유효 영역 확인
// ============================================================

bool isInsidePlatformArea(
    const Passenger& passenger)
{
    return passenger.x >= ZoneConfig::PLATFORM_X_MIN
        && passenger.x < ZoneConfig::PLATFORM_X_MAX
        && passenger.y >= ZoneConfig::PLATFORM_Y_MIN
        && passenger.y < ZoneConfig::PLATFORM_Y_MAX;
}


// ============================================================
// Core Zone 확인
// ============================================================

bool isInsideCoreZone(
    const Passenger& passenger,
    const PlatformZone& zone)
{
    return passenger.x >= zone.coreXMin
        && passenger.x < zone.coreXMax
        && passenger.y >= zone.coreYMin
        && passenger.y < zone.coreYMax;
}


// ============================================================
// Core Zone 찾기
// ============================================================

int findCoreZone(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones)
{
    for (std::size_t i = 0;
         i < zones.size();
         ++i)
    {
        if (isInsideCoreZone(
                passenger,
                zones[i]))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}


// ============================================================
// Passenger ↔ Door 거리
// ============================================================

double calculateDistanceToDoor(
    const Passenger& passenger,
    const PlatformZone& zone)
{
    double dx =
        static_cast<double>(
            passenger.x - zone.doorX);

    double dy =
        static_cast<double>(
            passenger.y - zone.doorY);

    return std::hypot(dx, dy);
}


// ============================================================
// Extended Queue 판정
// ============================================================

int findNearestExtendedZone(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones)
{
    if (!isInsidePlatformArea(passenger))
    {
        return -1;
    }


    double nearestDistance =
        std::numeric_limits<double>::max();

    double secondNearestDistance =
        std::numeric_limits<double>::max();

    int nearestIndex = -1;


    for (std::size_t i = 0;
         i < zones.size();
         ++i)
    {
        double distance =
            calculateDistanceToDoor(
                passenger,
                zones[i]);


        if (distance < nearestDistance)
        {
            secondNearestDistance =
                nearestDistance;

            nearestDistance =
                distance;

            nearestIndex =
                static_cast<int>(i);
        }
        else if (distance
                 < secondNearestDistance)
        {
            secondNearestDistance =
                distance;
        }
    }


    if (nearestIndex < 0)
    {
        return -1;
    }


    const PlatformZone& nearestZone =
        zones[
            static_cast<std::size_t>(
                nearestIndex)];


    // Door에서 너무 먼 경우
    if (nearestDistance
        > nearestZone.maxAssignmentDistance)
    {
        return -1;
    }


    // 두 Door 사이에서 너무 애매한 경우
    if (secondNearestDistance
            - nearestDistance
        < ZoneConfig::AMBIGUOUS_DISTANCE_MARGIN)
    {
        return -1;
    }


    return nearestIndex;
}


// ============================================================
// 최종 Passenger 분류
// ============================================================

int classifyPassenger(
    const Passenger& passenger,
    const std::array<PlatformZone, 3>& zones)
{
    // 승강장 자체를 벗어남
    if (!isInsidePlatformArea(passenger))
    {
        return -1;
    }


    // 1순위: Core Zone
    int coreZone =
        findCoreZone(
            passenger,
            zones);


    if (coreZone >= 0)
    {
        return coreZone;
    }


    // 2순위: Extended Queue
    return findNearestExtendedZone(
        passenger,
        zones);
}


// ============================================================
// 전체 Passenger Count
// ============================================================

int countPassengersInZones(
    const std::vector<Passenger>& passengers,
    std::array<PlatformZone, 3>& zones)
{
    for (auto& zone : zones)
    {
        zone.detectedModelPassengers = 0;
    }


    int unassignedPassengers = 0;


    for (const auto& passenger : passengers)
    {
        int zoneIndex =
            classifyPassenger(
                passenger,
                zones);


        if (zoneIndex >= 0)
        {
            ++zones[
                static_cast<std::size_t>(
                    zoneIndex)]
                .detectedModelPassengers;
        }
        else
        {
            ++unassignedPassengers;
        }
    }


    return unassignedPassengers;
}
