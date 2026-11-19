#include <iostream>
#include <limits>
#include <stdexcept>
#include "CoordinateMapper.hpp"
#include "MockInput.hpp"
#include "ZoneConfig.hpp"
#include "ZoneManager.hpp"

int main()
{
    CoordinateMapper identity;
    bool passed = true;
    std::vector<Passenger> canonical;
    for (const auto& passenger : createMockPassengers())
    {
        const auto mapped = identity.map({static_cast<double>(passenger.x),
                                          static_cast<double>(passenger.y)});
        passed = passed && mapped && mapped->x == passenger.x && mapped->y == passenger.y;
        if (mapped) canonical.push_back(*mapped);
    }
    auto zones = ZoneConfig::createDefaultZones();
    const auto unassigned = countPassengersInZones(canonical, zones);
    passed = passed && unassigned == 2 && zones[0].detectedModelPassengers == 4
        && zones[1].detectedModelPassengers == 3 && zones[2].detectedModelPassengers == 3;

    const auto scaled = CoordinateMapper::scaledFrame(1920, 1080);
    const auto middle = scaled.map({960, 540});
    const auto edge = scaled.map({1920, 1080});
    const auto outside = identity.map({1250, 400});
    passed = passed && middle && middle->x == 600 && middle->y == 300
        && edge && edge->x == 1200 && edge->y == 600
        && classifyPassenger(*edge, zones) == -1
        && outside && outside->x == 1250 && classifyPassenger(*outside, zones) == -1;

    const CoordinateMapper perspective{CoordinateMappingConfig{{1, 0, 0, 0, 1, 0, 0.001, 0, 1}}};
    const auto projected = perspective.map({1000, 400});
    passed = passed && projected && projected->x == 500 && projected->y == 200
        && !perspective.map({-1000, 400})
        && !identity.map({std::numeric_limits<double>::quiet_NaN(), 0})
        && !identity.map({std::numeric_limits<double>::infinity(), 0})
        && !identity.map({1e30, 0});
    bool singularRejected = false;
    try { CoordinateMapper invalid{CoordinateMappingConfig{{0, 0, 0, 0, 0, 0, 0, 0, 0}}}; }
    catch (const std::invalid_argument&) { singularRejected = true; }
    bool dimensionsRejected = false;
    try { CoordinateMapper::scaledFrame(0, 1080); }
    catch (const std::invalid_argument&) { dimensionsRejected = true; }
    passed = passed && singularRejected && dimensionsRejected;
    std::cout << (passed ? "CoordinateMapper tests: ALL PASSED\n"
                        : "CoordinateMapper tests: FAILED\n");
    return passed ? 0 : 1;
}
