#include "Win32SerialLineTransport.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>
#include <string_view>
#include "Win32SerialPort.hpp"

namespace
{
    std::wstring devicePath(std::string_view device)
    {
        if (device.starts_with(R"(\\.\)")) device.remove_prefix(4);
        if (device.size() > 3 && (device[0] == 'C' || device[0] == 'c')
            && (device[1] == 'O' || device[1] == 'o') && (device[2] == 'M' || device[2] == 'm'))
        {
            device.remove_prefix(3);
            unsigned int port{};
            const auto end = device.data() + device.size();
            const auto parsed = std::from_chars(device.data(), end, port);
            if (parsed.ec == std::errc{} && parsed.ptr == end && port > 0)
                return LR"(\\.\COM)" + std::to_wstring(port);
        }
        throw std::invalid_argument("Expected a COM port name such as COM3 or COM10");
    }
}

Win32SerialLineTransport::Win32SerialLineTransport(const std::string& device, SerialConfig config)
    : config_(config), lines_(config.maxLineBytes)
{
    if (config_.readTimeout.count() < 0 || config_.readTimeout.count() > 60000)
        throw std::invalid_argument("Serial timeout must be in 0..60000 ms");
    if (config_.baudRate == 0) throw std::invalid_argument("Serial baud rate must be positive");
    port_ = openWin32SerialPort(devicePath(device), config_.baudRate);
}

Win32SerialLineTransport::~Win32SerialLineTransport() = default;

void Win32SerialLineTransport::close() noexcept
{
    port_.reset();
    lines_.reset();
}

std::optional<ReceivedSerialLine> Win32SerialLineTransport::readLine()
{
    if (!isOpen()) throw std::logic_error("Serial transport is closed");
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    const auto deadline = Clock::now() + config_.readTimeout;
    std::array<char, 512> chunk{};
    bool firstRead = true;
    while (true)
    {
        if (config_.stopRequested && config_.stopRequested()) return std::nullopt;
        if (auto line = lines_.pop()) return line;
        const auto now = Clock::now();
        if (!firstRead && now >= deadline) return std::nullopt;
        firstRead = false;
        // Short driver waits allow the console thread's stop request to be observed promptly.
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
        const auto wait = std::clamp(remaining, 0ms, 100ms);
        Win32SerialRead received;
        try
        {
            received = port_->read(chunk, wait);
        }
        catch (...)
        {
            close();
            throw;
        }
        if (received.corrupted) lines_.discardUntilLineEnd();
        else lines_.push({chunk.data(), received.size}, Clock::now());
    }
}
