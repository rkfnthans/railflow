#pragma once

#include <deque>
#include <string_view>
#include "SerialLineTransport.hpp"

// Accumulates LF-delimited wire data across reads and timeouts.
class SerialLineBuffer
{
public:
    explicit SerialLineBuffer(std::size_t limit);
    void push(std::string_view bytes, std::chrono::steady_clock::time_point receivedAt);
    std::optional<ReceivedSerialLine> pop();
    void reset() noexcept;
    void discardUntilLineEnd() noexcept;
    std::size_t discardedOverlongLines() const noexcept { return discardedOverlong_; }

private:
    std::size_t limit_;
    std::string partial_;
    std::deque<ReceivedSerialLine> complete_;
    bool discarding_{};
    std::size_t discardedOverlong_{};
};
