#include "ConsoleSupport.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <limits>
#include <string>
#include <system_error>

namespace
{
    std::atomic<bool> stopped{false};

    BOOL WINAPI requestStop(DWORD event)
    {
        if (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT) return FALSE;
        stopped.store(true, std::memory_order_relaxed);
        return TRUE;
    }
}

void installConsoleStopHandler()
{
    if (!::SetConsoleCtrlHandler(requestStop, TRUE))
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), "SetConsoleCtrlHandler");
}

bool consoleStopRequested() noexcept { return stopped.load(std::memory_order_relaxed); }
#else
#include <csignal>
#include <stdexcept>

namespace
{
    volatile std::sig_atomic_t stopped = 0;
    void requestStop(int) { stopped = 1; }
}

void installConsoleStopHandler()
{
    if (std::signal(SIGINT, requestStop) == SIG_ERR || std::signal(SIGTERM, requestStop) == SIG_ERR)
        throw std::runtime_error("Cannot install stop handler");
}

bool consoleStopRequested() noexcept { return stopped != 0; }
#endif

void writeConsoleOutput(std::string_view text)
{
#ifdef _WIN32
    const HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode{};
    if (::GetConsoleMode(output, &mode))
    {
        std::string consoleText;
        consoleText.reserve(text.size());
        for (const char byte : text)
        {
            if (byte == '\n' && (consoleText.empty() || consoleText.back() != '\r')) consoleText.push_back('\r');
            consoleText.push_back(byte);
        }
        if (consoleText.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
            const auto bytes = static_cast<int>(consoleText.size());
            const int length = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, consoleText.data(), bytes, nullptr, 0);
            if (length > 0)
            {
                std::wstring wide(static_cast<std::size_t>(length), L'\0');
                if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, consoleText.data(), bytes, wide.data(), length) == length
                    && ::WriteConsoleW(output, wide.data(), static_cast<DWORD>(wide.size()), nullptr, nullptr)) return;
            }
        }
    }
#endif
    // Preserve UTF-8 bytes for files and pipes.
    std::cout << text;
}

void printCommandLineDiagnostics(int argc, char* argv[])
{
    std::ostringstream details;
    details << "Received argument bytes (hex):\n";
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument = argv[index];
        details << "  argv[" << index << "] length=" << argument.size() << ':';
        // The prefix identifies transformed dashes, quotes and other unexpected characters.
        for (const unsigned char byte : argument.substr(0, 96))
            details << ' ' << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);
        details << std::dec;
        if (argument.size() > 96) details << " ...";
        details << '\n';
    }
    std::cerr << details.str();
}
