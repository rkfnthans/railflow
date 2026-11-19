#include <exception>
#include <iostream>
#include <string_view>
#include <vector>
#include "CalibrationOptions.hpp"
#include "ConsoleSupport.hpp"
#include "NativeSerialLineTransport.hpp"

int main(int argc, char* argv[])
{
    try
    {
        const std::vector<std::string_view> args(argv + 1, argv + argc);
        const auto options = parseCalibrationOptions(args);
        if (options.help)
        {
            std::cout << "Usage: railflow_calibration_record <device> --vehicle CAR1 --weight-g 500\n"
                         "  --position CENTER --trial 1 --output data/calibration/car1.csv\n"
                         "  [--duration-sec 5] [--settle-ms 1000] [--samples N]\n"
                         "  [--baud 115200] [--session-id text] [--note text]\n"
                         "Uppercase vehicle/position only. Samples OR duration ends capture first.\n"
                         "Weight is a user-supplied label, never predicted from ADC.\n";
            return 0;
        }
        installConsoleStopHandler();
        SerialConfig config;
        config.baudRate = options.baudRate;
        config.stopRequested = consoleStopRequested;
        NativeSerialLineTransport transport(options.device, config);
        CalibrationDatasetRecorder recorder(options.output, options.metadata);
        std::cout << "Serial opened: " << options.device << " baud=" << options.baudRate
                  << "\nSettling: " << options.capture.settleTime.count() << " ms; measurement: "
                  << options.capture.duration.count() << " ms\n" << std::flush;
        const auto stats = runCalibrationCapture(transport, recorder, options.capture, consoleStopRequested);
        printCalibrationSummary(std::cout, options.metadata, stats, options.output);
        std::cout << "Overlong lines discarded (entire run): " << transport.discardedOverlongLines() << '\n';
        transport.close();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Calibration recorder: " << error.what() << '\n';
        printCommandLineDiagnostics(argc, argv);
        return 1;
    }
}
