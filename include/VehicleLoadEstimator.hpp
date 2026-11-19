#pragma once

#include "Vehicle.hpp"


double calculateModelLoadUnits(
    double passengerLoadG);


double calculateEquivalentPassengers(
    double modelUnits);


double calculateVehicleCrowding(
    double equivalentPassengers);


// Vehicle 구조체의 모든 값을 한 번에 계산
void updateVehicleLoad(
    Vehicle& vehicle);