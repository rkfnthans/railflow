#include "SerialFrameTracker.hpp"

SerialSequenceResult SerialFrameTracker::observe(std::uint64_t sequence)
{
    if (!last_)
    {
        last_ = sequence;
        return {SerialSequenceStatus::First, 0};
    }
    if (sequence == *last_) return {SerialSequenceStatus::Duplicate, 0};
    if (sequence < *last_) return {SerialSequenceStatus::Backward, 0};
    const auto difference = sequence - *last_;
    last_ = sequence;
    if (difference == 1) return {SerialSequenceStatus::Sequential, 0};
    return {SerialSequenceStatus::Skipped, difference - 1};
}

void SerialFrameTracker::reset()
{
    last_.reset();
}

std::optional<std::uint64_t> SerialFrameTracker::lastAcceptedSequence() const
{
    return last_;
}
