#include "PlatformCrowdEstimator.hpp"

#include "Config.hpp"


double calculatePlatformEquivalentPassengers(
    int detectedModelPassengers)
{
    if (detectedModelPassengers < 0)
    {
        return 0.0;
    }

    return static_cast<double>(
               detectedModelPassengers)
         * Config::REAL_PASSENGERS_PER_UNIT;
}


double calculatePlatformCrowding(
    int detectedModelPassengers)
{
    if (detectedModelPassengers < 0)
    {
        return 0.0;
    }

    return static_cast<double>(
               detectedModelPassengers)
         / Config::PLATFORM_MODEL_CAPACITY;
}


void updatePlatformCrowding(
    PlatformZone& zone)
{
    zone.equivalentPassengers =
        calculatePlatformEquivalentPassengers(
            zone.detectedModelPassengers);

    zone.crowding =
        calculatePlatformCrowding(
            zone.detectedModelPassengers);
}
