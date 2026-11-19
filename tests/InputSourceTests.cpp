#include <iostream>
#include "MockInput.hpp"

int main()
{
    MockInputSource mock;
    InputSource& source = mock;
    auto first = source.read();
    first.vehicles[0].passengerLoadG = 999.0;
    first.passengers.clear();
    const auto second = source.read();
    const auto vehicles = createMockVehicles();
    const auto passengers = createMockPassengers();
    bool passed = second.passengers.size() == passengers.size();
    for (std::size_t i = 0; i < vehicles.size(); ++i)
    {
        passed = passed && second.vehicles[i].name == vehicles[i].name
            && second.vehicles[i].passengerLoadG == vehicles[i].passengerLoadG
            && second.vehicles[i].crowding == 0.0;
    }
    if (second.passengers.size() == passengers.size())
    {
        for (std::size_t i = 0; i < passengers.size(); ++i)
        {
            passed = passed && second.passengers[i].x == passengers[i].x
                && second.passengers[i].y == passengers[i].y;
        }
    }
    std::cout << (passed ? "InputSource tests: ALL PASSED\n"
                        : "InputSource tests: FAILED\n");
    return passed ? 0 : 1;
}
