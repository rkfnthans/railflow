#pragma once

#include <string>

struct Vehicle
{
    std::string name;

    double passengerLoadG{};

    double modelLoadUnits{};

    double equivalentPassengers{};

    double crowding{};
};