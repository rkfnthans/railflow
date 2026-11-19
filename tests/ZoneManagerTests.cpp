#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "Passenger.hpp"
#include "PlatformZone.hpp"
#include "ZoneManager.hpp"
#include "ZoneConfig.hpp"


namespace
{
    struct ZoneTestCase
    {
        Passenger passenger;
        int expectedZoneIndex{};
        std::string description;
    };




    std::string zoneName(
        int zoneIndex,
        const std::array<PlatformZone, 3>& zones)
    {
        if (zoneIndex < 0)
        {
            return "UNASSIGNED";
        }

        return zones[
            static_cast<std::size_t>(zoneIndex)]
            .name;
    }
}


int main()
{
    auto zones = ZoneConfig::createDefaultZones();

    const std::vector<ZoneTestCase> tests{
        {{200, 520}, 0, "P1 Core"},
        {{600, 520}, 1, "P2 Core"},
        {{1000, 520}, 2, "P3 Core"},
        {{140, 300}, 0, "P1 긴 대기열"},
        {{620, 300}, 1, "P2 긴 대기열"},
        {{1000, 240}, 2, "P3 긴 대기열"},
        {{340, 420}, 0, "P1 옆쪽 동행"},
        {{720, 400}, 1, "P2 옆쪽 동행"},
        {{1120, 380}, 2, "P3 옆쪽 동행"},
        {{400, 500}, -1, "P1/P2 중간"},
        {{800, 500}, -1, "P2/P3 중간"},
        {{200, 150}, -1, "승강장 뒤쪽 범위 밖"},
        {{1250, 400}, -1, "카메라 승강장 영역 밖"}
    };

    int passedTests = 0;

    for (const auto& test : tests)
    {
        const int actualZoneIndex =
            classifyPassenger(
                test.passenger,
                zones);

        if (actualZoneIndex == test.expectedZoneIndex)
        {
            std::cout
                << "[PASS] "
                << test.description
                << "\n";

            ++passedTests;
        }
        else
        {
            std::cout
                << "[FAIL] "
                << test.description
                << " | expected: "
                << zoneName(
                       test.expectedZoneIndex,
                       zones)
                << ", actual: "
                << zoneName(
                       actualZoneIndex,
                       zones)
                << "\n";
        }
    }

    const std::vector<Passenger> passengers{
        {180, 520},
        {250, 480},
        {140, 300},
        {340, 420},
        {600, 500},
        {520, 320},
        {720, 400},
        {980, 500},
        {920, 320},
        {1120, 380},
        {400, 500},
        {200, 150}
    };

    const int unassignedPassengers =
        countPassengersInZones(
            passengers,
            zones);

    const bool countTestPassed =
        zones[0].detectedModelPassengers == 4
        && zones[1].detectedModelPassengers == 3
        && zones[2].detectedModelPassengers == 3
        && unassignedPassengers == 2;

    if (countTestPassed)
    {
        std::cout
            << "[PASS] countPassengersInZones"
            << " | P1=4, P2=3, P3=3, UNASSIGNED=2\n";

        ++passedTests;
    }
    else
    {
        std::cout
            << "[FAIL] countPassengersInZones"
            << " | expected: P1=4, P2=3, P3=3, UNASSIGNED=2"
            << ", actual: P1="
            << zones[0].detectedModelPassengers
            << ", P2="
            << zones[1].detectedModelPassengers
            << ", P3="
            << zones[2].detectedModelPassengers
            << ", UNASSIGNED="
            << unassignedPassengers
            << "\n";
    }

    const int totalTests =
        static_cast<int>(tests.size()) + 1;

    if (passedTests == totalTests)
    {
        std::cout
            << "ZoneManager tests: ALL PASSED\n";

        return 0;
    }

    std::cout
        << "ZoneManager tests: FAILED\n";

    return 1;
}
