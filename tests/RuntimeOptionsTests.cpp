#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include "RuntimeOptions.hpp"

int main()
{
    const auto parse = [](std::initializer_list<std::string_view> args)
    {
        return parseRuntimeOptions({args.begin(), args.size()});
    };
    const auto invalid = [&](std::initializer_list<std::string_view> args)
    {
        try { parse(args); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    const auto defaults = parse({});
    const auto demo = parse({"--demo"});
    const auto bounded = parse({"--demo", "--frames", "3", "--period-ms", "10", "--no-log"});
    bool passed = defaults.loop.maxFrames == 1 && defaults.writeCsv
        && demo.loop.maxFrames == 0 && bounded.loop.maxFrames == 3
        && bounded.loop.period.count() == 10 && !bounded.writeCsv
        && parse({"--help"}).help
        && parse({"--layout"}).showLayout
        && invalid({"--layout", "--demo"})
        && invalid({"--layout", "--no-log"})
        && invalid({"--layout", "--help"})
        && invalid({"--demo", "--frames"}) && invalid({"--frames", "3"})
        && invalid({"--demo", "--period-ms", "0"})
        && invalid({"--demo", "--frames", "-1"})
        && invalid({"--demo", "--frames", "3x"})
        && invalid({"--demo", "--frames", "1000001"})
        && invalid({"--demo", "--period-ms", "60001"})
        && invalid({"--unknown"});
    std::cout << (passed ? "RuntimeOptions tests: ALL PASSED\n" : "RuntimeOptions tests: FAILED\n");
    return passed ? 0 : 1;
}
