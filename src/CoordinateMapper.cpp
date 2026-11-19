#include "CoordinateMapper.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include "ZoneConfig.hpp"

CoordinateMapper::CoordinateMapper(CoordinateMappingConfig config) : config_(config)
{
    const auto& h = config_.homography;
    for (const double value : h)
    {
        if (!std::isfinite(value)) throw std::invalid_argument("non-finite homography");
    }
    const double determinant = h[0] * (h[4] * h[8] - h[5] * h[7])
        - h[1] * (h[3] * h[8] - h[5] * h[6])
        + h[2] * (h[3] * h[7] - h[4] * h[6]);
    if (!std::isfinite(determinant) || determinant == 0.0)
    {
        throw std::invalid_argument("singular or unrepresentable homography");
    }
}

CoordinateMapper CoordinateMapper::scaledFrame(double rawWidth, double rawHeight)
{
    if (!std::isfinite(rawWidth) || !std::isfinite(rawHeight)
        || rawWidth <= 0.0 || rawHeight <= 0.0)
    {
        throw std::invalid_argument("raw frame dimensions must be finite and positive");
    }
    CoordinateMappingConfig config;
    config.homography[0] = ZoneConfig::FRAME_WIDTH / rawWidth;
    config.homography[4] = ZoneConfig::FRAME_HEIGHT / rawHeight;
    return CoordinateMapper{config};
}

std::optional<Passenger> CoordinateMapper::map(RawPoint point) const
{
    if (!std::isfinite(point.x) || !std::isfinite(point.y)) return std::nullopt;
    const auto& h = config_.homography;
    const double denominator = h[6] * point.x + h[7] * point.y + h[8];
    if (!std::isfinite(denominator) || denominator == 0.0) return std::nullopt;
    const double x = std::round((h[0] * point.x + h[1] * point.y + h[2]) / denominator);
    const double y = std::round((h[3] * point.x + h[4] * point.y + h[5]) / denominator);
    const auto valid = [](double value)
    {
        return std::isfinite(value) && value >= std::numeric_limits<int>::min()
            && value <= std::numeric_limits<int>::max();
    };
    if (!valid(x) || !valid(y)) return std::nullopt;
    return Passenger{static_cast<int>(x), static_cast<int>(y)};
}
