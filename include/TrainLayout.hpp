#pragma once

#include <cstddef>

// Capstone mock-up geometry and wiring order. Values here describe the physical
// model; they are not the camera's 1200x600 canonical coordinate system.
namespace TrainLayout
{
    inline constexpr std::size_t CAR_COUNT = 3;
    inline constexpr std::size_t FSR_PER_CAR = 3;
    inline constexpr std::size_t TOTAL_FSR_COUNT = CAR_COUNT * FSR_PER_CAR;

    inline constexpr double CAR_LENGTH_CM = 25.0; // train length direction
    inline constexpr double CAR_WIDTH_CM = 15.0;
    inline constexpr double CAR_HEIGHT_CM = 15.0; // approximate target height
    inline constexpr double TOTAL_BODY_LENGTH_CM = CAR_COUNT * CAR_LENGTH_CM;

    // Three sensors per car lie in ONE ROW along the 25 cm length axis.
    // Exact gaps/coordinates are NOT yet specified; do not assume equal spacing.
    // Zero-based indices: (car, sensor) -> RF1 adc field / CD74HC4067 C channel.
    constexpr std::size_t channelIndex(std::size_t car, std::size_t sensor)
    {
        return car * FSR_PER_CAR + sensor;
    }
    constexpr std::size_t carIndex(std::size_t channel)
    {
        return channel / FSR_PER_CAR;
    }
    constexpr std::size_t sensorIndex(std::size_t channel)
    {
        return channel % FSR_PER_CAR;
    }

    static_assert(CAR_COUNT == 3 && FSR_PER_CAR == 3 && TOTAL_FSR_COUNT == 9);
    static_assert(channelIndex(0, 0) == 0 && channelIndex(1, 0) == 3
                  && channelIndex(2, 2) == 8);
}
