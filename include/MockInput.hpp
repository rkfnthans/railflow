#pragma once

#include <array>
#include <vector>

#include "Passenger.hpp"
#include "Vehicle.hpp"
#include "InputSource.hpp"


std::array<Vehicle, 3> createMockVehicles();


std::vector<Passenger> createMockPassengers();

class MockInputSource final : public InputSource
{
public:
    InputFrame read() override;
};
