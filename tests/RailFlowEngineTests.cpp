#include <cmath>
#include <iostream>
#include <sstream>
#include "ConsoleReporter.hpp"
#include "GuidanceEngine.hpp"
#include "MockInput.hpp"
#include "RailFlowEngine.hpp"
#include "ZoneConfig.hpp"

int main()
{
    const auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    MockInputSource mock;
    const auto first = updateRailFlow(mock.read(), ZoneConfig::createDefaultZones());
    bool passed = first.unassignedPassengers == 2 && first.bestZoneIndex == 2;
    const std::array<int, 3> counts{4, 3, 3};
    const std::array<double, 3> current{0.4, 0.7, 0.2};
    const std::array<double, 3> predicted{0.8, 1.0, 0.5};
    for (std::size_t i = 0; i < counts.size(); ++i)
    {
        passed = passed && first.zones[i].detectedModelPassengers == counts[i]
            && near(first.input.vehicles[i].crowding, current[i])
            && near(first.zones[i].predictedVehicleCrowding, predicted[i]);
    }
    auto empty = mock.read();
    empty.passengers.clear();
    empty.vehicles[0].passengerLoadG = 2500.0;
    empty.vehicles[2].passengerLoadG = 1500.0;
    const auto second = updateRailFlow(empty, first.zones);
    passed = passed && second.unassignedPassengers == 0 && second.bestZoneIndex == 1
        && near(second.input.vehicles[0].crowding, 2.5)
        && near(second.zones[0].predictedVehicleCrowding, 2.5)
        && getCrowdingStatus(second.zones[0].predictedVehicleCrowding) == "초과혼잡";
    for (const auto& zone : second.zones)
    {
        passed = passed && zone.detectedModelPassengers == 0
            && near(zone.equivalentPassengers, 0.0) && near(zone.crowding, 0.0);
    }
    std::ostringstream output;
    printRailFlow(first, output);
    passed = passed && output.str().find("추천 승차 위치 : P3") != std::string::npos;
    std::cout << (passed ? "RailFlowEngine tests: ALL PASSED\n"
                        : "RailFlowEngine tests: FAILED\n");
    return passed ? 0 : 1;
}
