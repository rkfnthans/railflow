#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "PlatformCrowdEstimator.hpp"
#include "PlatformZone.hpp"


namespace
{
    constexpr double TOLERANCE = 1e-9;

    struct TestCase
    {
        int detectedModelPassengers{};
        double expectedEquivalentPassengers{};
        double expectedCrowding{};
    };


    bool nearlyEqual(
        double actual,
        double expected)
    {
        return std::abs(actual - expected)
            < TOLERANCE;
    }


    bool reportResult(
        bool passed,
        const std::string& description,
        double actual,
        double expected)
    {
        std::cout
            << (passed ? "[PASS] " : "[FAIL] ")
            << description;

        if (!passed)
        {
            std::cout
                << " | expected: "
                << expected
                << ", actual: "
                << actual;
        }

        std::cout << "\n";

        return passed;
    }
}


int main()
{
    const std::vector<TestCase> tests{
        {0, 0.0, 0.0},
        {1, 16.0, 0.1},
        {4, 64.0, 0.4},
        {10, 160.0, 1.0},
        {12, 192.0, 1.2},
        {-1, 0.0, 0.0}
    };

    int passedTests = 0;

    for (const auto& test : tests)
    {
        const double actualEquivalentPassengers =
            calculatePlatformEquivalentPassengers(
                test.detectedModelPassengers);

        const bool equivalentPassed =
            nearlyEqual(
                actualEquivalentPassengers,
                test.expectedEquivalentPassengers);

        if (reportResult(
                equivalentPassed,
                std::to_string(
                    test.detectedModelPassengers)
                    + " passengers equivalent count",
                actualEquivalentPassengers,
                test.expectedEquivalentPassengers))
        {
            ++passedTests;
        }

        const double actualCrowding =
            calculatePlatformCrowding(
                test.detectedModelPassengers);

        const bool crowdingPassed =
            nearlyEqual(
                actualCrowding,
                test.expectedCrowding);

        if (reportResult(
                crowdingPassed,
                std::to_string(
                    test.detectedModelPassengers)
                    + " passengers crowding",
                actualCrowding,
                test.expectedCrowding))
        {
            ++passedTests;
        }
    }

    PlatformZone zone{};
    zone.detectedModelPassengers = 4;

    updatePlatformCrowding(zone);

    const bool updateEquivalentPassed =
        nearlyEqual(
            zone.equivalentPassengers,
            64.0);

    if (reportResult(
            updateEquivalentPassed,
            "updatePlatformCrowding equivalent count",
            zone.equivalentPassengers,
            64.0))
    {
        ++passedTests;
    }

    const bool updateCrowdingPassed =
        nearlyEqual(
            zone.crowding,
            0.4);

    if (reportResult(
            updateCrowdingPassed,
            "updatePlatformCrowding crowding",
            zone.crowding,
            0.4))
    {
        ++passedTests;
    }

    const int totalTests =
        static_cast<int>(tests.size()) * 2 + 2;

    if (passedTests == totalTests)
    {
        std::cout
            << "PlatformCrowdEstimator tests: ALL PASSED\n";

        return 0;
    }

    std::cout
        << "PlatformCrowdEstimator tests: FAILED\n";

    return 1;
}
