#include <array>
#include <cstddef>
#include <iostream>
#include <string>

#include "MockInput.hpp"
#include "Passenger.hpp"


namespace
{
    bool reportResult(
        bool passed,
        const std::string& description)
    {
        std::cout
            << (passed ? "[PASS] " : "[FAIL] ")
            << description
            << "\n";

        return passed;
    }
}


int main()
{
    bool allPassed = true;

    const auto vehicles =
        createMockVehicles();

    allPassed =
        reportResult(
            vehicles.size() == 3,
            "vehicle count")
        && allPassed;

    allPassed =
        reportResult(
            vehicles[0].name == "CAR1"
                && vehicles[0].passengerLoadG == 400.0,
            "CAR1 mock load")
        && allPassed;

    allPassed =
        reportResult(
            vehicles[1].name == "CAR2"
                && vehicles[1].passengerLoadG == 700.0,
            "CAR2 mock load")
        && allPassed;

    allPassed =
        reportResult(
            vehicles[2].name == "CAR3"
                && vehicles[2].passengerLoadG == 200.0,
            "CAR3 mock load")
        && allPassed;

    const auto passengers =
        createMockPassengers();

    allPassed =
        reportResult(
            passengers.size() == 12,
            "passenger count")
        && allPassed;

    const std::array<Passenger, 12> expectedPassengers{{
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
    }};

    bool passengerCoordinatesMatch =
        passengers.size()
        == expectedPassengers.size();

    if (passengerCoordinatesMatch)
    {
        for (std::size_t i = 0;
             i < expectedPassengers.size();
             ++i)
        {
            if (passengers[i].x
                    != expectedPassengers[i].x
                || passengers[i].y
                    != expectedPassengers[i].y)
            {
                passengerCoordinatesMatch = false;
                break;
            }
        }
    }

    allPassed =
        reportResult(
            passengerCoordinatesMatch,
            "passenger coordinates and order")
        && allPassed;

    if (allPassed)
    {
        std::cout
            << "MockInput tests: ALL PASSED\n";

        return 0;
    }

    std::cout
        << "MockInput tests: FAILED\n";

    return 1;
}
