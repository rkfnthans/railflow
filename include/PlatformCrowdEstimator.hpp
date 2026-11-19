#pragma once

#include "PlatformZone.hpp"


double calculatePlatformEquivalentPassengers(
    int detectedModelPassengers);


double calculatePlatformCrowding(
    int detectedModelPassengers);


void updatePlatformCrowding(
    PlatformZone& zone);
