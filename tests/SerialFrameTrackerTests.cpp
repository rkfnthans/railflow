#include <iostream>
#include <limits>
#include "SerialFrameTracker.hpp"

int main()
{
    int passed = 0;
    int total = 0;
    const auto check = [&](bool result, const char* name)
    {
        ++total;
        passed += result;
        std::cout << (result ? "[PASS] " : "[FAIL] ") << name << '\n';
    };
    SerialFrameTracker tracker;
    check(!tracker.lastAcceptedSequence(), "initially no sequence");
    auto result = tracker.observe(10);
    check(result.status == SerialSequenceStatus::First && result.skippedFrames == 0
          && tracker.lastAcceptedSequence() == 10, "first frame");
    result = tracker.observe(11);
    check(result.status == SerialSequenceStatus::Sequential && result.skippedFrames == 0,
          "10 to 11 sequential");
    result = tracker.observe(11);
    check(result.status == SerialSequenceStatus::Duplicate && result.skippedFrames == 0
          && tracker.lastAcceptedSequence() == 11, "duplicate does not update baseline");
    result = tracker.observe(14);
    check(result.status == SerialSequenceStatus::Skipped && result.skippedFrames == 2
          && tracker.lastAcceptedSequence() == 14, "11 to 14 skips two frames");
    result = tracker.observe(13);
    check(result.status == SerialSequenceStatus::Backward && result.skippedFrames == 0
          && tracker.lastAcceptedSequence() == 14, "backward does not update baseline");
    check(tracker.observe(14).status == SerialSequenceStatus::Duplicate, "duplicate after backward");
    check(tracker.observe(15).status == SerialSequenceStatus::Sequential, "normal after backward");
    tracker.reset();
    check(!tracker.lastAcceptedSequence() && tracker.observe(0).status == SerialSequenceStatus::First,
          "explicit reset accepts new session starting at zero");
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    result = tracker.observe(maximum);
    check(result.status == SerialSequenceStatus::Skipped && result.skippedFrames == maximum - 1,
          "large gap without overflow");
    check(tracker.observe(maximum).status == SerialSequenceStatus::Duplicate, "maximum duplicate");
    check(tracker.observe(0).status == SerialSequenceStatus::Backward
          && tracker.lastAcceptedSequence() == maximum, "wrap is backward until explicit reset");
    tracker.reset();
    tracker.observe(maximum - 1);
    check(tracker.observe(maximum).status == SerialSequenceStatus::Sequential,
          "last uint64 increment is sequential");
    std::cout << "SerialFrameTracker checks: " << passed << '/' << total << '\n';
    return passed == total ? 0 : 1;
}
