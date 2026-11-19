#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "UpdateLoop.hpp"

int main()
{
    using namespace std::chrono;
    LoopClock::Clock::time_point now{};
    std::vector<LoopClock::Clock::time_point> ticks;
    const LoopClock clock{[&] { return now; }, [&](auto deadline) { now = deadline; }};
    const auto origin = now;
    const auto count = runUpdateLoop({milliseconds{100}, 3},
        [&] { ticks.push_back(now); }, [] { return false; }, clock);
    bool passed = count == 3 && ticks.size() == 3
        && ticks[0] == origin && ticks[1] == origin + milliseconds{100}
        && ticks[2] == origin + milliseconds{200};

    now = origin;
    ticks.clear();
    runUpdateLoop({milliseconds{100}, 2}, [&]
    {
        ticks.push_back(now);
        now += milliseconds{250};
    }, [] { return false; }, clock);
    passed = passed && ticks.size() == 2 && ticks[1] == origin + milliseconds{300};

    int updates = 0;
    const auto stopped = runUpdateLoop({milliseconds{1}, 0}, [&] { ++updates; },
        [&] { return updates == 4; }, clock);
    passed = passed && stopped == 4;
    const auto preStopped = runUpdateLoop({milliseconds{1}, 0}, [&] { ++updates; },
        [] { return true; }, clock);
    passed = passed && preStopped == 0 && updates == 4;

    bool invalidRejected = false;
    try { runUpdateLoop({milliseconds{0}, 1}, [] {}, [] { return false; }, clock); }
    catch (const std::invalid_argument&) { invalidRejected = true; }
    bool failurePropagated = false;
    try
    {
        runUpdateLoop({milliseconds{1}, 1}, [] { throw std::runtime_error("input unavailable"); },
                      [] { return false; }, clock);
    }
    catch (const std::runtime_error&) { failurePropagated = true; }
    passed = passed && invalidRejected && failurePropagated;
    std::cout << (passed ? "UpdateLoop tests: ALL PASSED\n" : "UpdateLoop tests: FAILED\n");
    return passed ? 0 : 1;
}
