#pragma once

#include <chrono>
#include <cstddef>
#include <functional>

struct LoopOptions
{
    std::chrono::milliseconds period{1000};
    std::size_t maxFrames{1}; // 0이면 종료 요청까지 계속 실행
};

struct LoopClock
{
    using Clock = std::chrono::steady_clock;
    std::function<Clock::time_point()> now;
    std::function<void(Clock::time_point)> sleepUntil;
};

LoopClock steadyLoopClock();

// 첫 프레임은 즉시 실행. 지연된 주기는 건너뛰고 과거 입력을 재실행하지 않는다.
std::size_t runUpdateLoop(const LoopOptions& options,
                          const std::function<void()>& update,
                          const std::function<bool()>& stopRequested,
                          const LoopClock& clock);
