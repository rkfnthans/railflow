#pragma once

#include <cstdint>
#include <optional>

enum class SerialSequenceStatus
{
    First,
    Sequential,
    Duplicate,
    Skipped,
    Backward
};

struct SerialSequenceResult
{
    SerialSequenceStatus status;
    std::uint64_t skippedFrames{};
};

class SerialFrameTracker
{
public:
    // 검증이 끝난 frame의 sequence만 전달한다.
    // Duplicate/Backward는 기준값을 갱신하지 않는다.
    SerialSequenceResult observe(std::uint64_t sequence);
    void reset(); // 장치 재시작/재연결 정책은 호출자가 결정한다.
    std::optional<std::uint64_t> lastAcceptedSequence() const;

private:
    std::optional<std::uint64_t> last_;
};
