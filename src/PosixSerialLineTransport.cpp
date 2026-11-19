#include "PosixSerialLineTransport.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

namespace
{
    speed_t speedFor(unsigned int baud)
    {
        switch (baud)
        {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: throw std::invalid_argument("Unsupported serial baud rate");
        }
    }

    [[noreturn]] void osError(const char* operation)
    {
        throw std::system_error(errno, std::generic_category(), operation);
    }
}

PosixSerialLineTransport::PosixSerialLineTransport(const std::string& device, PosixSerialConfig config)
    : config_(config), lines_(config.maxLineBytes)
{
    const speed_t speed = speedFor(config_.baudRate);
    if (config_.readTimeout.count() < 0 || config_.readTimeout.count() > 60000)
        throw std::invalid_argument("Serial timeout must be in 0..60000 ms");
    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) osError("open serial device");
    try
    {
        if (::tcgetattr(fd_, &original_) < 0) osError("tcgetattr");
        haveOriginal_ = true;
        termios settings = original_;
        ::cfmakeraw(&settings);
        settings.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
        settings.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
        settings.c_cflag &= ~CRTSCTS;
#endif
        settings.c_iflag &= ~(IXON | IXOFF | IXANY);
        settings.c_cc[VMIN] = 1;
        settings.c_cc[VTIME] = 0;
        if (::cfsetispeed(&settings, speed) < 0 || ::cfsetospeed(&settings, speed) < 0)
            osError("set serial baud");
        if (::tcsetattr(fd_, TCSANOW, &settings) < 0) osError("tcsetattr");
    }
    catch (...)
    {
        close();
        throw;
    }
}

PosixSerialLineTransport::~PosixSerialLineTransport() { close(); }

void PosixSerialLineTransport::close() noexcept
{
    if (fd_ >= 0)
    {
        if (haveOriginal_) ::tcsetattr(fd_, TCSANOW, &original_);
        // Do not retry close on EINTR: the descriptor may already have been released.
        ::close(fd_);
        fd_ = -1;
    }
    lines_.reset();
}

std::optional<ReceivedSerialLine> PosixSerialLineTransport::readLine()
{
    return readLine(config_.readTimeout);
}

std::optional<ReceivedSerialLine> PosixSerialLineTransport::readLine(std::chrono::milliseconds timeout)
{
    if (!isOpen()) throw std::logic_error("Serial transport is closed");
    if (timeout.count() < 0 || timeout.count() > 60000)
        throw std::invalid_argument("Serial timeout must be in 0..60000 ms");
    using Clock = std::chrono::steady_clock;
    const auto deadline = Clock::now() + timeout;
    bool firstPoll = true;
    while (true)
    {
        if (config_.stopRequested && config_.stopRequested()) return std::nullopt;
        if (auto line = lines_.pop()) return line;
        const auto now = Clock::now();
        if (!firstPoll && now >= deadline) return std::nullopt;
        firstPoll = false;
        auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - now).count();
        if (config_.stopRequested) remaining = std::min(remaining, decltype(remaining){100});
        const int waitMs = static_cast<int>(std::max(remaining, decltype(remaining){0}));
        pollfd port{fd_, POLLIN, 0};
        const int ready = ::poll(&port, 1, waitMs);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            osError("poll serial device");
        }
        if (ready == 0) continue;
        if (port.revents & POLLNVAL) throw std::runtime_error("Invalid serial descriptor");
        if (port.revents & POLLIN)
        {
            // Read only one bounded chunk before draining the completed-line queue.
            std::array<char, 512> chunk{};
            const auto size = ::read(fd_, chunk.data(), chunk.size());
            if (size > 0)
            {
                lines_.push({chunk.data(), static_cast<std::size_t>(size)}, Clock::now());
                continue;
            }
            if (size == 0) throw std::runtime_error("Serial device disconnected (EOF)");
            if (errno == EINTR) continue;
            if (errno != EAGAIN && errno != EWOULDBLOCK) osError("read serial device");
        }
        if (port.revents & (POLLHUP | POLLERR))
            throw std::runtime_error("Serial device disconnected or unavailable");
    }
}
