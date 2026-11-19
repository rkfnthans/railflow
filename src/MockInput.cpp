#include "MockInput.hpp"

InputFrame MockInputSource::read()
{
    return {createMockVehicles(), createMockPassengers()};
}


std::array<Vehicle, 3> createMockVehicles()
{
    return {{
        {"CAR1", 400.0},
        {"CAR2", 700.0},
        {"CAR3", 200.0}
    }};
}


std::vector<Passenger> createMockPassengers()
{
    return {
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
}
