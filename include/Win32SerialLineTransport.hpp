#pragma once

#include <memory>
#include "SerialLineBuffer.hpp"

class Win32SerialPort;

class Win32SerialLineTransport final : public SerialLineTransport
{
public:
    explicit Win32SerialLineTransport(const std::string& device, SerialConfig config = {});
    ~Win32SerialLineTransport() override;
    Win32SerialLineTransport(const Win32SerialLineTransport&) = delete;
    Win32SerialLineTransport& operator=(const Win32SerialLineTransport&) = delete;

    std::optional<ReceivedSerialLine> readLine() override;
    void close() noexcept;
    bool isOpen() const noexcept { return static_cast<bool>(port_); }
    std::size_t discardedOverlongLines() const noexcept { return lines_.discardedOverlongLines(); }

private:
    SerialConfig config_;
    SerialLineBuffer lines_;
    std::unique_ptr<Win32SerialPort> port_;
};
