#include <chrono>
#include <iostream>
#include <utility>
#include <vector>
#include "SerialFrameParser.hpp"
#include "SerialFrameTracker.hpp"
#include "SerialLineTransport.hpp"

namespace
{
    // 테스트 전용 transport. production 코드에 mock replay를 혼합하지 않는다.
    class ReplayLines final : public SerialLineTransport
    {
    public:
        explicit ReplayLines(std::vector<ReceivedSerialLine> lines) : lines_(std::move(lines)) {}
        std::optional<ReceivedSerialLine> readLine() override
        {
            if (next_ == lines_.size()) return std::nullopt;
            return lines_[next_++];
        }
    private:
        std::vector<ReceivedSerialLine> lines_;
        std::size_t next_{};
    };
}

int main()
{
    using Clock = std::chrono::steady_clock;
    const Clock::time_point firstTime{std::chrono::seconds{100}};
    const auto secondTime = firstTime + std::chrono::milliseconds{25};
    ReplayLines replay{{
        {"RF1,10,4294967295,1,2,3,4,5,6,7,8,9\n", firstTime},
        {"RF1,999,0,1024,2,3,4,5,6,7,8,9\n", secondTime},
        {"RF1,11,0,9,8,7,6,5,4,3,2,1\r\n", secondTime}
    }};
    SerialLineTransport& transport = replay;
    SerialFrameTracker tracker;
    std::vector<ReceivedFsrRawFrame> received;
    int invalidLines = 0;
    bool sequenceCorrect = true;
    while (const auto line = transport.readLine())
    {
        const auto parsed = parseFsrSerialFrame(line->text);
        if (!parsed)
        {
            ++invalidLines;
            continue;
        }
        received.push_back({*parsed, line->receivedAt});
        const auto status = tracker.observe(parsed->sequence).status;
        sequenceCorrect = sequenceCorrect && status == (received.size() == 1
            ? SerialSequenceStatus::First : SerialSequenceStatus::Sequential);
    }
    const bool passed = sequenceCorrect && invalidLines == 1 && received.size() == 2
        && received[0].receivedAt == firstTime && received[1].receivedAt == secondTime
        && received[0].frame.deviceMillis == 4294967295U && received[1].frame.deviceMillis == 0
        && received[0].frame.adc == std::array<int, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}
        && tracker.lastAcceptedSequence() == 11 && !transport.readLine();
    std::cout << (passed ? "SerialTransport tests: ALL PASSED\n" : "SerialTransport tests: FAILED\n");
    return passed ? 0 : 1;
}
