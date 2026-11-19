#pragma once

#include <string>

struct PlatformZone
{
    std::string name;

    // 출입문 기준 위치
    int doorX{};
    int doorY{};

    // 확실한 대기 영역
    int coreXMin{};
    int coreXMax{};

    int coreYMin{};
    int coreYMax{};

    // Extended Queue 최대 허용거리
    double maxAssignmentDistance{};

    // 검출 결과
    int detectedModelPassengers{};

    // 환산 승객수
    double equivalentPassengers{};

    // 승강장 혼잡도
    double crowding{};

    // 승차 후 예상 차량 혼잡도
    double predictedVehicleCrowding{};
};