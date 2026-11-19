#include "VehicleLoadEstimator.hpp"
#include "Config.hpp"



// ============================================================
// 승객 하중(g)
// → 모형 하중 단위
// ============================================================

double calculateModelLoadUnits(
    double passengerLoadG)
{
    if (passengerLoadG < 0.0)
    {
        return 0.0;
    }

    return passengerLoadG
         / Config::MODEL_LOAD_UNIT_G;
}


// ============================================================
// 모형 하중 단위
// → 실제 환산 승객
// ============================================================

double calculateEquivalentPassengers(
    double modelUnits)
{
    if (modelUnits < 0.0)
    {
        return 0.0;
    }

    return modelUnits
         * Config:: REAL_PASSENGERS_PER_UNIT;
}


// ============================================================
// 환산 승객
// → 차량 혼잡도
// ============================================================

double calculateVehicleCrowding(
    double equivalentPassengers)
{
    if (equivalentPassengers < 0.0)
    {
        return 0.0;
    }

    return equivalentPassengers
         / Config::VEHICLE_CAPACITY;
}


// ============================================================
// Vehicle 전체 상태 갱신
// ============================================================

void updateVehicleLoad(
    Vehicle& vehicle)
{
    vehicle.modelLoadUnits =
        calculateModelLoadUnits(
            vehicle.passengerLoadG);


    vehicle.equivalentPassengers =
        calculateEquivalentPassengers(
            vehicle.modelLoadUnits);


    vehicle.crowding =
        calculateVehicleCrowding(
            vehicle.equivalentPassengers);
}