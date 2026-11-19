#include <array>
#include <cmath>
#include <iostream>
#include <string>

#include "ZoneConfig.hpp"

int main()
{
    int passed = 0;
    int total = 0;
    const auto check = [&](bool matches, const std::string& description)
    {
        ++total;
        passed += matches;
        std::cout << (matches ? "[PASS] " : "[FAIL] ") << description << '\n';
    };
    const auto near = [](double actual, double expected)
    {
        return std::abs(actual - expected) < 1e-9;
    };

    check(ZoneConfig::FRAME_WIDTH == 1200, "canonical width");
    check(ZoneConfig::FRAME_HEIGHT == 600, "canonical height");
    check(ZoneConfig::PLATFORM_X_MIN == 0, "platform x minimum");
    check(ZoneConfig::PLATFORM_X_MAX == 1200, "platform x maximum");
    check(ZoneConfig::PLATFORM_Y_MIN == 220, "platform y minimum");
    check(ZoneConfig::PLATFORM_Y_MAX == 600, "platform y maximum");
    check(near(ZoneConfig::AMBIGUOUS_DISTANCE_MARGIN, 30.0), "ambiguity margin");

    auto zones = ZoneConfig::createDefaultZones();
    check(zones.size() == 3, "default zone count");
    const std::array<PlatformZone, 3> expected{{
        {"P1", 200, 560, 100, 300, 440, 600, 360.0},
        {"P2", 600, 560, 500, 700, 440, 600, 360.0},
        {"P3", 1000, 560, 900, 1100, 440, 600, 360.0}
    }};

    for (std::size_t i = 0; i < zones.size(); ++i)
    {
        const auto& zone = zones[i];
        const auto& reference = expected[i];
        const auto field = [&](bool matches, const char* name)
        {
            check(matches, reference.name + " " + name);
        };
        field(zone.name == reference.name, "name");
        field(zone.doorX == reference.doorX, "doorX");
        field(zone.doorY == reference.doorY, "doorY");
        field(zone.coreXMin == reference.coreXMin, "coreXMin");
        field(zone.coreXMax == reference.coreXMax, "coreXMax");
        field(zone.coreYMin == reference.coreYMin, "coreYMin");
        field(zone.coreYMax == reference.coreYMax, "coreYMax");
        field(near(zone.maxAssignmentDistance, reference.maxAssignmentDistance),
              "maxAssignmentDistance");
        field(zone.detectedModelPassengers == 0, "initial detectedModelPassengers");
        field(near(zone.equivalentPassengers, 0.0), "initial equivalentPassengers");
        field(near(zone.crowding, 0.0), "initial crowding");
        field(near(zone.predictedVehicleCrowding, 0.0), "initial predictedVehicleCrowding");
    }

    // 생성할 때마다 독립적인 런타임 상태를 반환한다.
    zones[0].detectedModelPassengers = 4;
    check(ZoneConfig::createDefaultZones()[0].detectedModelPassengers == 0,
          "independent default zone state");

    std::cout << "Checks passed: " << passed << '/' << total << '\n';
    std::cout << (passed == total
        ? "ZoneConfig tests: ALL PASSED\n"
        : "ZoneConfig tests: FAILED\n");
    return passed == total ? 0 : 1;
}
