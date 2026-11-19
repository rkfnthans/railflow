#include "RailFlowEngine.hpp"

#include <utility>
#include "GuidanceEngine.hpp"
#include "PlatformCrowdEstimator.hpp"
#include "VehicleLoadEstimator.hpp"
#include "ZoneManager.hpp"

RailFlowState updateRailFlow(InputFrame input, std::array<PlatformZone, 3> zones)
{
    RailFlowState state{std::move(input), std::move(zones)};
    state.unassignedPassengers = countPassengersInZones(state.input.passengers, state.zones);
    for (auto& vehicle : state.input.vehicles)
    {
        updateVehicleLoad(vehicle);
    }
    for (auto& zone : state.zones)
    {
        updatePlatformCrowding(zone);
    }
    updatePredictedCrowding(state.input.vehicles, state.zones);
    state.bestZoneIndex = findBestZone(state.zones);
    return state;
}
