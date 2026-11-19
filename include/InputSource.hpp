#pragma once

#include <array>
#include <vector>

#include "Passenger.hpp"
#include "Vehicle.hpp"

// 한 번 읽은 차량 하중과 canonical 좌표. 파생 상태는 core에서 계산한다.
struct InputFrame
{
    std::array<Vehicle, 3> vehicles;
    std::vector<Passenger> passengers;
};

class InputSource
{
public:
    virtual ~InputSource() = default;
    // 입력을 읽을 수 없으면 예외를 발생시킨다. 빈 승객 목록은 유효한 무인 상태다.
    virtual InputFrame read() = 0;
};
