#include "SerialLineBuffer.hpp"

#include <stdexcept>
#include <utility>

SerialLineBuffer::SerialLineBuffer(std::size_t limit) : limit_(limit)
{
    if (limit_ == 0 || limit_ > 65536) throw std::invalid_argument("Invalid serial line limit");
}

void SerialLineBuffer::push(std::string_view bytes, std::chrono::steady_clock::time_point receivedAt)
{
    for (const char byte : bytes)
    {
        if (byte == '\n')
        {
            if (!discarding_)
            {
                if (!partial_.empty() && partial_.back() == '\r') partial_.pop_back();
                complete_.push_back({std::move(partial_), receivedAt});
            }
            partial_.clear();
            discarding_ = false;
        }
        else if (!discarding_)
        {
            if (partial_.size() == limit_)
            {
                discardUntilLineEnd();
                ++discardedOverlong_;
            }
            else partial_.push_back(byte);
        }
    }
}

std::optional<ReceivedSerialLine> SerialLineBuffer::pop()
{
    if (complete_.empty()) return std::nullopt;
    auto line = std::move(complete_.front());
    complete_.pop_front();
    return line;
}

void SerialLineBuffer::reset() noexcept
{
    complete_.clear();
    partial_.clear();
    discarding_ = false;
}

void SerialLineBuffer::discardUntilLineEnd() noexcept
{
    partial_.clear();
    discarding_ = true;
}
