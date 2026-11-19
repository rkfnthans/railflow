#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <span>
#include <string>

struct Win32SerialRead
{
    std::size_t size{};
    bool corrupted{}; // Driver reported damaged data and the receive buffer was purged.
};

class Win32SerialPort
{
public:
    virtual ~Win32SerialPort() = default;
    virtual Win32SerialRead read(std::span<char> buffer, std::chrono::milliseconds timeout) = 0;
};

std::unique_ptr<Win32SerialPort> openWin32SerialPort(const std::wstring& path, unsigned int baudRate);
