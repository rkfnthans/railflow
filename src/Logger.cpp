#include "Logger.hpp"

#include <fstream>
#include <iostream>
#include <string>


void writeLog(
    const std::array<Vehicle, 3>& vehicles,
    const std::array<PlatformZone, 3>& zones,
    std::size_t bestZoneIndex,
    int unassignedPassengers)
{
    const std::string filePath =
        "data/logs/railflow_log.csv";


    // 기존 파일이 존재하고
    // 데이터가 들어 있는지 확인
    std::ifstream checkFile(filePath);


    bool fileExistsAndHasData =
        checkFile.good()
        &&
        checkFile.peek()
            != std::ifstream::traits_type::eof();


    checkFile.close();


    // 기존 CSV 뒤에 새로운 행 추가
    std::ofstream logFile(
        filePath,
        std::ios::app);


    if (!logFile)
    {
        std::cerr
            << "로그 파일을 열 수 없습니다.\n";

        return;
    }


    // 새 파일이면 Header 생성
    if (!fileExistsAndHasData)
    {
        logFile
            << "car1_load_g,"
            << "car2_load_g,"
            << "car3_load_g,"

            << "p1_count,"
            << "p2_count,"
            << "p3_count,"

            << "unassigned_count,"

            << "car1_equivalent_passengers,"
            << "car2_equivalent_passengers,"
            << "car3_equivalent_passengers,"

            << "car1_crowding_percent,"
            << "car2_crowding_percent,"
            << "car3_crowding_percent,"

            << "predicted1_percent,"
            << "predicted2_percent,"
            << "predicted3_percent,"

            << "recommended_zone\n";
    }


    // 실험 데이터 기록
    logFile
        << vehicles[0].passengerLoadG << ","
        << vehicles[1].passengerLoadG << ","
        << vehicles[2].passengerLoadG << ","

        << zones[0].detectedModelPassengers << ","
        << zones[1].detectedModelPassengers << ","
        << zones[2].detectedModelPassengers << ","

        << unassignedPassengers << ","

        << vehicles[0].equivalentPassengers << ","
        << vehicles[1].equivalentPassengers << ","
        << vehicles[2].equivalentPassengers << ","

        << vehicles[0].crowding * 100.0 << ","
        << vehicles[1].crowding * 100.0 << ","
        << vehicles[2].crowding * 100.0 << ","

        << zones[0].predictedVehicleCrowding * 100.0 << ","
        << zones[1].predictedVehicleCrowding * 100.0 << ","
        << zones[2].predictedVehicleCrowding * 100.0 << ","

        << zones[bestZoneIndex].name

        << "\n";
}