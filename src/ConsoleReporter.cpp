#include "ConsoleReporter.hpp"

#include <iomanip>
#include "Config.hpp"
#include "GuidanceEngine.hpp"
#include "ZoneManager.hpp"

void printRailFlow(const RailFlowState& state, std::ostream& output)
{
    const auto& vehicles = state.input.vehicles;
    const auto& passengers = state.input.passengers;
    const auto& zones = state.zones;
    const auto bestZoneIndex = state.bestZoneIndex;
    const auto unassignedPassengers = state.unassignedPassengers;

    // ========================================================
    // 9. 출력 설정
    // ========================================================

    output
        << std::fixed
        << std::setprecision(1);


    output
    << Config::MODEL_LOAD_UNIT_G
    << " g = 실제 승객 "
    << Config::REAL_PASSENGERS_PER_UNIT
    << "명 상당\n"

    << "1량 정원 = "
    << Config::VEHICLE_CAPACITY
    << "명\n"

    << Config::MODEL_LOAD_UNIT_G
           * (Config::VEHICLE_CAPACITY
              / Config::REAL_PASSENGERS_PER_UNIT)
    << " g = 혼잡도 100%\n";


    // ========================================================
    // 10. 승객별 Zone 판정 출력
    // ========================================================

    output
        << "\n=== 승객 위치 분류 ===\n";


    for (const auto& passenger : passengers)
    {
        int zoneIndex =
            classifyPassenger(
                passenger,
                zones);


        output
            << "("
            << passenger.x
            << ", "
            << passenger.y
            << ") -> ";


        if (zoneIndex >= 0)
        {
            output
                << zones[
                    static_cast<std::size_t>(
                        zoneIndex)]
                    .name;
        }
        else
        {
            output
                << "UNASSIGNED";
        }


        output << "\n";
    }


    // ========================================================
    // 11. Zone Count
    // ========================================================

    output
        << "\n=== Zone 자동 Count ===\n";


    for (const auto& zone : zones)
    {
        output
            << zone.name
            << " : "
            << zone.detectedModelPassengers
            << "명\n";
    }


    output
        << "UNASSIGNED : "
        << unassignedPassengers
        << "명\n";


    // ========================================================
    // 12. 차량 상태
    // ========================================================

    output
        << "\n=== 현재 차량 상태 ===\n";


    for (const auto& vehicle : vehicles)
    {
        output
            << vehicle.name
            << "\n"

            << "  승객 하중      : "
            << vehicle.passengerLoadG
            << " g\n"

            << "  모형 하중 단위 : "
            << vehicle.modelLoadUnits
            << "\n"

            << "  환산 승객      : "
            << vehicle.equivalentPassengers
            << "명\n"

            << "  현재 혼잡도    : "
            << vehicle.crowding * 100.0
            << "% ("
            << getCrowdingStatus(
                   vehicle.crowding)
            << ")\n\n";
    }


    // ========================================================
    // 13. 승강장 상태
    // ========================================================

    output
        << "=== 현재 승강장 상태 ===\n";


    for (const auto& zone : zones)
    {
        output
            << zone.name
            << "\n"

            << "  검출 모형 승객 : "
            << zone.detectedModelPassengers
            << "명\n"

            << "  환산 승객      : "
            << zone.equivalentPassengers
            << "명\n"

            << "  승강장 혼잡도  : "
            << zone.crowding * 100.0
            << "%\n\n";
    }


    // ========================================================
    // 14. 승차 후 예상 혼잡도
    // ========================================================

    output
        << "=== 승차 후 예상 차량 혼잡도 ===\n";


    for (std::size_t i = 0;
         i < vehicles.size();
         ++i)
    {
        output
            << vehicles[i].name
            << " + "
            << zones[i].name
            << "\n"

            << "  예상 혼잡도 : "
            << zones[i]
                   .predictedVehicleCrowding
                   * 100.0
            << "% ("
            << getCrowdingStatus(
                   zones[i]
                       .predictedVehicleCrowding)
            << ")\n\n";
    }


    // ========================================================
    // 15. 추천 승차 위치
    // ========================================================

    output
        << "=====================================\n"

        << "추천 승차 위치 : "
        << zones[bestZoneIndex].name
        << "\n"

        << "예상 혼잡도    : "
        << zones[bestZoneIndex]
               .predictedVehicleCrowding
               * 100.0
        << "%\n"

        << "=====================================\n\n";


}
