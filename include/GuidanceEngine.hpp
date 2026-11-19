#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "PlatformZone.hpp"
#include "Vehicle.hpp"


// 차량 현재 상태 + 승강장 대기 승객
// → 각 차량의 승차 후 예상 혼잡도 갱신
void updatePredictedCrowding(
    const std::array<Vehicle, 3>& vehicles,
    std::array<PlatformZone, 3>& zones);


// 예상 혼잡도가 가장 낮은 Zone 검색
std::size_t findBestZone(
    const std::array<PlatformZone, 3>& zones);


// 혼잡도 숫자를 사람이 읽을 수 있는 상태로 변환
std::string getCrowdingStatus(
    double crowding);