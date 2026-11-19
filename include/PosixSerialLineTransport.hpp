#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <termios.h>
#include "SerialLineBuffer.hpp"

using PosixSerialConfig = SerialConfig;

class PosixSerialLineTransport final : public SerialLineTransport
{
public:
    explicit PosixSerialLineTransport(const std::string& device, PosixSerialConfig config = {});
    ~PosixSerialLineTransport() override;
    PosixSerialLineTransport(const PosixSerialLineTransport&) = delete;
    PosixSerialLineTransport& operator=(const PosixSerialLineTransport&) = delete;

    std::optional<ReceivedSerialLine> readLine() override;
    std::optional<ReceivedSerialLine> readLine(std::chrono::milliseconds timeout);
    void close() noexcept;
    bool isOpen() const noexcept { return fd_ >= 0; }
    std::size_t discardedOverlongLines() const noexcept { return lines_.discardedOverlongLines(); }

private:
    int fd_{-1};
    termios original_{};
    bool haveOriginal_{};
    PosixSerialConfig config_;
    SerialLineBuffer lines_;
};
