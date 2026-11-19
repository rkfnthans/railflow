#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

struct SerialConfig
{
    unsigned int baudRate{115200};
    std::chrono::milliseconds readTimeout{100};
    // Maximum wire content before LF, including an optional CR.
    std::size_t maxLineBytes{512};
    bool (*stopRequested)() noexcept = nullptr;
};

struct ReceivedSerialLine
{
    // wire의 LF가 수신된 완성된 줄. terminator는 제거해도 된다.
    std::string text;
    std::chrono::steady_clock::time_point receivedAt;
};

// OS serial API와 parser 사이의 경계. 실제 포트를 열지 않는다.
class SerialLineTransport
{
public:
    virtual ~SerialLineTransport() = default;
    // 완성된 줄이 없으면 nullopt, I/O 오류는 예외.
    // 구현체는 partial byte를 모으고 overlong line을 LF까지 폐기해야 한다.
    virtual std::optional<ReceivedSerialLine> readLine() = 0;
};
