#include <iostream>
#include <sstream>
#include <string>
#include "FsrRawFrame.hpp"
#include "TrainLayout.hpp"
#include "TrainLayoutReporter.hpp"

int main()
{
    bool ok = TrainLayout::CAR_COUNT == 3
        && TrainLayout::FSR_PER_CAR == 3
        && TrainLayout::TOTAL_FSR_COUNT == 9
        && FsrProtocol::CHANNEL_COUNT == TrainLayout::TOTAL_FSR_COUNT
        && TrainLayout::CAR_LENGTH_CM == 30.0
        && TrainLayout::CAR_WIDTH_CM == 15.0
        && TrainLayout::CAR_HEIGHT_CM == 15.0
        && TrainLayout::TOTAL_BODY_LENGTH_CM == 90.0;
    bool used[TrainLayout::TOTAL_FSR_COUNT]{};
    for (std::size_t car = 0; car < TrainLayout::CAR_COUNT; ++car)
    {
        for (std::size_t sensor = 0; sensor < TrainLayout::FSR_PER_CAR; ++sensor)
        {
            const auto channel = TrainLayout::channelIndex(car, sensor);
            ok = ok && channel < TrainLayout::TOTAL_FSR_COUNT && !used[channel]
                    && TrainLayout::carIndex(channel) == car
                    && TrainLayout::sensorIndex(channel) == sensor;
            used[channel] = true;
        }
    }
    for (bool seen : used) ok = ok && seen;
    std::ostringstream output;
    printTrainLayout(output);
    const auto text = output.str();
    ok = ok && text.find("30 x 15 x approx. 15 cm") != std::string::npos
            && text.find("Total body length (excluding couplers/gaps): 90 cm") != std::string::npos
            && text.find("Top view: each row runs along the 30 cm length axis.") != std::string::npos
            && text.find("CAR1 [FSR1/C0/adc0 -- FSR2/C1/adc1 -- FSR3/C2/adc2]") != std::string::npos
            && text.find("CAR2 [FSR1/C3/adc3 -- FSR2/C4/adc4 -- FSR3/C5/adc5]") != std::string::npos
            && text.find("CAR3 [FSR1/C6/adc6 -- FSR2/C7/adc7 -- FSR3/C8/adc8]") != std::string::npos;
    std::cout << (ok ? "TrainLayout tests: ALL PASSED\n" : "TrainLayout tests: FAILED\n");
    return ok ? 0 : 1;
}
