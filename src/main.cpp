#include <iostream>
#include <exception>
#include <sstream>
#include <string_view>
#include <vector>
#include "ConsoleReporter.hpp"
#include "ConsoleSupport.hpp"
#include "Logger.hpp"
#include "MockInput.hpp"
#include "RailFlowEngine.hpp"
#include "ZoneConfig.hpp"
#include "RuntimeOptions.hpp"
#include "UpdateLoop.hpp"
#include "TrainLayoutReporter.hpp"

int main(int argc, char* argv[])
{
    try
    {
        const std::vector<std::string_view> arguments(argv + 1, argv + argc);
        const auto options = parseRuntimeOptions(arguments);
        if (options.help)
        {
            std::cout << "Usage: railflow [--demo [--frames N] [--period-ms N]] [--no-log]\n"
                      << "       railflow --layout\n"
                      << "Default: one frame. Demo: continuous; --frames 0 also means continuous.\n"
                      << "Period: 1..60000 ms, frames: 0..1000000. Ctrl+C stops demo.\n";
            return 0;
        }
        if (options.showLayout)
        {
            printTrainLayout(std::cout);
            return 0;
        }
        installConsoleStopHandler();
        writeConsoleOutput("입력 모드: Mock 시뮬레이션\n");
        MockInputSource input;
        const auto zones = ZoneConfig::createDefaultZones();
        runUpdateLoop(options.loop, [&]
        {
            const auto state = updateRailFlow(input.read(), zones);
            std::ostringstream report;
            printRailFlow(state, report);
            writeConsoleOutput(report.str());
            if (options.writeCsv)
            {
                writeLog(state.input.vehicles, state.zones,
                         state.bestZoneIndex, state.unassignedPassengers);
                writeConsoleOutput("결과를 data/logs/railflow_log.csv에 저장했습니다.\n\n");
            }
        }, consoleStopRequested, steadyLoopClock());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "RailFlow: " << error.what() << '\n';
        printCommandLineDiagnostics(argc, argv);
        return 1;
    }
}
