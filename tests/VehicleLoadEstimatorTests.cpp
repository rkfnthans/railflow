#include <array>
#include <cmath>
#include <iostream>

#include "Config.hpp"
#include "VehicleLoadEstimator.hpp"

// 정원 기준과 환산식이 같은 하중을 의미하는지 확인한다.
static_assert(Config::NOMINAL_FULL_LOAD_G
              == Config::MODEL_LOAD_UNIT_G * Config::VEHICLE_CAPACITY
                 / Config::REAL_PASSENGERS_PER_UNIT);

namespace
{
    struct TestCase
    {
        double loadG;
        double modelUnits;
        double equivalentPassengers;
        double crowding;
    };

    bool check(const char* field, double loadG, double actual, double expected)
    {
        const bool passed = std::abs(actual - expected) < 1e-9;
        std::cout << (passed ? "[PASS] " : "[FAIL] ")
                  << loadG << " g " << field;
        if (!passed)
        {
            std::cout << " | expected: " << expected << ", actual: " << actual;
        }
        std::cout << '\n';
        return passed;
    }
}

int main()
{
    const std::array<TestCase, 10> tests{{
        {-100.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0},
        {100.0, 1.0, 16.0, 0.1},
        {500.0, 5.0, 80.0, 0.5},
        {1000.0, 10.0, 160.0, 1.0},
        {1100.0, 11.0, 176.0, 1.1},
        {1200.0, 12.0, 192.0, 1.2},
        {1500.0, 15.0, 240.0, 1.5},
        {2000.0, 20.0, 320.0, 2.0},
        // 목표 검증 범위를 넘어도 하중과 혼잡도를 제한하지 않는다.
        {2500.0, 25.0, 400.0, 2.5}
    }};

    int passed = 0;
    for (const auto& test : tests)
    {
        const double modelUnits = calculateModelLoadUnits(test.loadG);
        const double passengers = calculateEquivalentPassengers(modelUnits);
        const double crowding = calculateVehicleCrowding(passengers);
        passed += check("modelLoadUnits", test.loadG, modelUnits, test.modelUnits);
        passed += check("equivalentPassengers", test.loadG,
                        passengers, test.equivalentPassengers);
        passed += check("crowding", test.loadG, crowding, test.crowding);
    }

    Vehicle vehicle{"TEST", 1200.0};
    updateVehicleLoad(vehicle);
    passed += check("update modelLoadUnits", 1200.0, vehicle.modelLoadUnits, 12.0);
    passed += check("update equivalentPassengers", 1200.0,
                    vehicle.equivalentPassengers, 192.0);
    passed += check("update crowding", 1200.0, vehicle.crowding, 1.2);

    const int total = static_cast<int>(tests.size()) * 3 + 3;
    std::cout << "Checks passed: " << passed << '/' << total << '\n';
    std::cout << (passed == total
        ? "VehicleLoadEstimator tests: ALL PASSED\n"
        : "VehicleLoadEstimator tests: FAILED\n");
    return passed == total ? 0 : 1;
}
