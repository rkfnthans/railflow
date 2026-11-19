#include "GuidanceEngine.hpp"
#include "Config.hpp"


namespace
{
    // ========================================================
    // 한 차량의 승차 후 예상 혼잡도
    //
    // 이 함수는 GuidanceEngine 내부에서만 사용한다.
    // ========================================================

    double calculatePredictedCrowding(
        double currentVehiclePassengers,
        double waitingPassengers)
    {
        double boardingPassengers =
            waitingPassengers
            * Config::BOARDING_RATIO;


        double predictedPassengers =
            currentVehiclePassengers
            + boardingPassengers;


        return predictedPassengers
             / Config::VEHICLE_CAPACITY;
    }
}


// ============================================================
// CAR1 ↔ P1
// CAR2 ↔ P2
// CAR3 ↔ P3
//
// 각각 승차 후 예상 혼잡도를 계산
// ============================================================

void updatePredictedCrowding(
    const std::array<Vehicle, 3>& vehicles,
    std::array<PlatformZone, 3>& zones)
{
    for (std::size_t i = 0;
         i < vehicles.size();
         ++i)
    {
        zones[i].predictedVehicleCrowding =
            calculatePredictedCrowding(
                vehicles[i].equivalentPassengers,
                zones[i].equivalentPassengers);
    }
}


// ============================================================
// 가장 예상 혼잡도가 낮은 Zone 찾기
// ============================================================

std::size_t findBestZone(
    const std::array<PlatformZone, 3>& zones)
{
    std::size_t bestIndex = 0;


    for (std::size_t i = 1;
         i < zones.size();
         ++i)
    {
        if (zones[i].predictedVehicleCrowding
            <
            zones[bestIndex].predictedVehicleCrowding)
        {
            bestIndex = i;
        }
    }


    return bestIndex;
}


// ============================================================
// 혼잡도 상태
// ============================================================

std::string getCrowdingStatus(
    double crowding)
{
    if (crowding < 0.4)
    {
        return "여유";
    }

    if (crowding < 0.7)
    {
        return "보통";
    }

    if (crowding <= 1.0)
    {
        return "혼잡";
    }

    return "초과혼잡";
}