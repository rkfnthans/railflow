#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include "TrainLayout.hpp"

namespace FsrProtocol
{
    inline constexpr std::size_t CHANNEL_COUNT = TrainLayout::TOTAL_FSR_COUNT;
    inline constexpr int ADC_MAX = 1023;
    // RF1 + uint64 sequence + uint32 millis + 9 ADC + 11 commas.
    inline constexpr std::size_t MAX_LINE_CONTENT_BYTES = 80;
}

struct FsrRawFrame
{
    std::uint64_t sequence{};
    std::uint32_t deviceMillis{};
    // Channel mapping is TrainLayout::channelIndex(car, sensor), zero-based.
    std::array<int, FsrProtocol::CHANNEL_COUNT> adc{};
};

struct ReceivedFsrRawFrame
{
    FsrRawFrame frame;
    // PC에서 완성된 한 줄을 수신한 시각. deviceMillis와 다른 clock domain이다.
    std::chrono::steady_clock::time_point receivedAt;
};
