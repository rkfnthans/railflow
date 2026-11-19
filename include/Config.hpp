#pragma once

namespace Config
{
    // 약 100 g = 모형 승객 1명 기준
    inline constexpr double MODEL_LOAD_UNIT_G = 100.0;

    // 100 g = 실제 승객 16명 상당
    inline constexpr double REAL_PASSENGERS_PER_UNIT = 16.0;

    // 실제 차량 1량 기준 정원
    inline constexpr double VEHICLE_CAPACITY = 160.0;

    // 차량 정원 100%에 해당하는 하중이며 센서 최대 측정값이 아님
    inline constexpr double NOMINAL_FULL_LOAD_G = 1000.0;

    // 차량당 목표 검증 범위이며 입력을 제한하는 상한이 아님
    inline constexpr double SENSOR_DESIGN_MAX_G = 2000.0;

    // 승강장 모형 승객 10명 = 100%
    inline constexpr int PLATFORM_MODEL_CAPACITY = 10;

    // 현재는 대기 승객 전원이 해당 차량에
    // 탑승한다고 가정
    inline constexpr double BOARDING_RATIO = 1.0;
}
