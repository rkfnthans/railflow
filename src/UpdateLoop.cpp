#include "UpdateLoop.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

LoopClock steadyLoopClock()
{
    return {[] { return LoopClock::Clock::now(); },
            [](auto deadline) { std::this_thread::sleep_until(deadline); }};
}

std::size_t runUpdateLoop(const LoopOptions& options,
                          const std::function<void()>& update,
                          const std::function<bool()>& stopRequested,
                          const LoopClock& clock)
{
    if (options.period.count() <= 0)
    {
        throw std::invalid_argument("update period must be positive");
    }
    std::size_t frames = 0;
    auto deadline = clock.now();
    while (!stopRequested())
    {
        update();
        ++frames;
        if (options.maxFrames != 0 && frames >= options.maxFrames)
        {
            break;
        }
        deadline += options.period;
        const auto now = clock.now();
        if (deadline < now)
        {
            const auto missed = (now - deadline) / options.period + 1;
            deadline += options.period * missed;
        }
        // 종료 신호를 긴 주기에서도 빠르게 확인한다.
        while (clock.now() < deadline && !stopRequested())
        {
            clock.sleepUntil(std::min(deadline, clock.now() + std::chrono::milliseconds{20}));
        }
    }
    return frames;
}
