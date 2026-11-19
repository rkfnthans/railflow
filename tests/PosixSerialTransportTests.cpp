#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include "PosixSerialLineTransport.hpp"
#include "SerialFrameParser.hpp"

namespace
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    const std::string valid = "RF1,42,15320,231,228,235,410,398,405,120,118,121";
    int passed = 0;
    int failed = 0;
    void check(bool condition, const char* label)
    {
        if (condition) ++passed;
        else { ++failed; std::cerr << "FAILED: " << label << '\n'; }
    }
    template<typename Function> bool throws(Function action)
    {
        try { action(); } catch (const std::exception&) { return true; }
        return false;
    }

    class Pty
    {
    public:
        Pty()
        {
            master_ = ::posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
            if (master_ < 0) throw std::runtime_error("posix_openpt failed");
            if (::grantpt(master_) < 0 || ::unlockpt(master_) < 0 || !::ptsname(master_))
            {
                disconnect();
                throw std::runtime_error("PTY setup failed");
            }
            path = ::ptsname(master_);
        }
        ~Pty() { disconnect(); }
        Pty(const Pty&) = delete;
        Pty& operator=(const Pty&) = delete;
        void send(const std::string& bytes)
        {
            std::size_t offset = 0;
            while (offset < bytes.size())
            {
                const auto size = ::write(master_, bytes.data() + offset, bytes.size() - offset);
                if (size < 0 && errno == EINTR) continue;
                if (size <= 0) throw std::runtime_error("PTY write failed");
                offset += static_cast<std::size_t>(size);
            }
        }
        void disconnect()
        {
            if (master_ >= 0) { ::close(master_); master_ = -1; }
        }
        std::string path;
    private:
        int master_{-1};
    };

    volatile std::sig_atomic_t interrupted = 0;
    void onSignal(int) { interrupted = 1; }
}

int main()
{
    try
    {
        Pty pty;
        PosixSerialLineTransport port(pty.path, {115200, 20ms, 128});
        check(port.isOpen(), "open");
        const auto before = Clock::now();
        pty.send(valid + '\n');
        auto line = port.readLine();
        check(line && line->text == valid, "complete LF line");
        check(line && line->receivedAt >= before && line->receivedAt <= Clock::now(), "host receive clock");
        const auto frame = line ? parseFsrSerialFrame(line->text) : std::nullopt;
        check(frame && frame->adc == std::array<int, 9>{231,228,235,410,398,405,120,118,121},
              "firmware RF1 sample and CAR channel order");

        pty.send("RF1,42,");
        check(!port.readLine(), "partial line timeout");
        pty.send("15320,231,228,");
        check(!port.readLine(5ms), "partial line second chunk");
        pty.send("235,410,398,405,120,118,121\n");
        line = port.readLine();
        check(line && line->text == valid, "partial chunks combined");

        pty.send("one\ntwo\r\nthree\n");
        const auto one = port.readLine();
        const auto two = port.readLine(0ms);
        const auto three = port.readLine(0ms);
        check(one && one->text == "one", "multiple lines first");
        check(two && two->text == "two", "CRLF stripping and queued zero timeout");
        check(three && three->text == "three", "multiple lines last");

        const auto start = Clock::now();
        check(!port.readLine(25ms), "normal timeout returns nullopt");
        const auto elapsed = Clock::now() - start;
        check(elapsed >= 15ms && elapsed < 1s, "bounded timeout");
        pty.send("RF2,garbage\n");
        line = port.readLine();
        check(line && line->text == "RF2,garbage" && !parseFsrSerialFrame(line->text),
              "transport preserves malformed RF1 for parser rejection");

        pty.send(std::string(128, 'a') + '\n');
        line = port.readLine();
        check(line && line->text.size() == 128, "exact line limit allowed");
        // Feed incrementally: a PTY writer has its own finite kernel queue.
        // A large synchronous write before the reader starts can block the test itself.
        for (int chunk = 0; chunk < 64; ++chunk)
        {
            pty.send(std::string(256, 'x'));
            check(!port.readLine(1ms), "overlong chunk produces no line");
        }
        check(!port.readLine(), "unterminated overlong line discarded without growth");
        check(port.discardedOverlongLines() == 1, "overlong counted once");
        pty.send("tail\n" + valid + "\r\n");
        line = port.readLine();
        check(line && line->text == valid, "resynchronization after overlong LF");

        struct sigaction handler{};
        struct sigaction previous{};
        handler.sa_handler = onSignal;
        (void)sigemptyset(&handler.sa_mask); // macOS exposes this as a macro.
        if (::sigaction(SIGUSR1, &handler, &previous) < 0) throw std::runtime_error("sigaction failed");
        const auto reader = ::pthread_self();
        {
            std::jthread writer([&]
            {
                std::this_thread::sleep_for(10ms);
                ::pthread_kill(reader, SIGUSR1);
                std::this_thread::sleep_for(10ms);
                pty.send("after EINTR\n");
            });
            line = port.readLine(500ms);
        }
        ::sigaction(SIGUSR1, &previous, nullptr);
        check(interrupted != 0 && line && line->text == "after EINTR", "EINTR retries within deadline");

        check(throws([&] { port.readLine(-1ms); }), "invalid timeout rejected");
        port.close();
        port.close();
        check(!port.isOpen(), "idempotent close");
        check(throws([&] { port.readLine(); }), "closed is not timeout");

        Pty disconnectPty;
        PosixSerialLineTransport disconnected(disconnectPty.path);
        disconnectPty.disconnect();
        check(throws([&] { disconnected.readLine(100ms); }), "disconnect is error, not timeout");
        check(throws([] { PosixSerialLineTransport missing("/railflow/nonexistent/serial-device"); }), "open failure");
        check(throws([&] { PosixSerialLineTransport bad(pty.path, {12345, 1ms, 128}); }), "unsupported baud");
        check(throws([&] { PosixSerialLineTransport bad(pty.path, {115200, 1ms, 0}); }), "invalid line limit");
    }
    catch (const std::exception& error)
    {
        ++failed;
        std::cerr << "Unexpected exception: " << error.what() << '\n';
    }
    std::cout << "PosixSerialTransport tests: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
