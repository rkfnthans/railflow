#include "TrainLayoutReporter.hpp"

#include <ostream>
#include "TrainLayout.hpp"

void printTrainLayout(std::ostream& output)
{
    output << "RailFlow capstone train model\n"
           << "Cars: " << TrainLayout::CAR_COUNT << "\n"
           << "Each car (length x width x height): "
           << TrainLayout::CAR_LENGTH_CM << " x "
           << TrainLayout::CAR_WIDTH_CM << " x approx. "
           << TrainLayout::CAR_HEIGHT_CM << " cm\n"
           << "Total body length (excluding couplers/gaps): "
           << TrainLayout::TOTAL_BODY_LENGTH_CM << " cm\n"
           << "FSRs per car: " << TrainLayout::FSR_PER_CAR << "\n"
           << "Total FSRs: " << TrainLayout::TOTAL_FSR_COUNT << "\n"
           << "Top view: each row runs along the " << TrainLayout::CAR_LENGTH_CM
           << " cm length axis.\n";

    for (std::size_t car = 0; car < TrainLayout::CAR_COUNT; ++car)
    {
        output << "CAR" << car + 1 << " [";
        for (std::size_t sensor = 0; sensor < TrainLayout::FSR_PER_CAR; ++sensor)
        {
            if (sensor != 0) output << " -- ";
            const auto channel = TrainLayout::channelIndex(car, sensor);
            output << "FSR" << sensor + 1 << "/C" << channel << "/adc" << channel;
        }
        output << "]\n";
    }
    output << "Note: exact sensor gaps and mounting coordinates are not yet fixed.\n"
           << "RF1 sends raw ADC (0..1023), NOT calibrated grams.\n";
}
