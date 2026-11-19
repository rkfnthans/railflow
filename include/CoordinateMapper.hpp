#pragma once

#include <array>
#include <optional>
#include "Passenger.hpp"

struct RawPoint
{
    double x{};
    double y{};
};

struct CoordinateMappingConfig
{
    // row-major 3x3: raw 평면에서 RailFlow canonical 평면으로 변환.
    std::array<double, 9> homography{1, 0, 0, 0, 1, 0, 0, 0, 1};
};

class CoordinateMapper
{
public:
    explicit CoordinateMapper(CoordinateMappingConfig config = {});
    // 원근 보정 없이 전체 raw 화면 크기만 canonical 크기로 변환한다.
    static CoordinateMapper scaledFrame(double rawWidth, double rawHeight);
    // 최근접 정수 좌표 반환. 범위 밖 좌표는 clamp하지 않는다.
    // 비유한 값, 투영 불가능한 점, 정수 범위 초과는 nullopt.
    std::optional<Passenger> map(RawPoint point) const;

private:
    CoordinateMappingConfig config_;
};
