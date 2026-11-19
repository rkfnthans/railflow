#include "SerialFrameParser.hpp"

#include <array>
#include <charconv>

namespace
{
    template <typename Integer>
    bool parseUnsigned(std::string_view text, Integer& value)
    {
        if (text.empty()) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size();
    }
}

std::optional<FsrRawFrame> parseFsrSerialFrame(std::string_view line)
{
    if (line.size() > FsrProtocol::MAX_LINE_CONTENT_BYTES + 2) return std::nullopt;
    if (!line.empty() && line.back() == '\n')
    {
        line.remove_suffix(1);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    }
    if (line.size() > FsrProtocol::MAX_LINE_CONTENT_BYTES) return std::nullopt;

    std::array<std::string_view, 3 + FsrProtocol::CHANNEL_COUNT> fields;
    for (std::size_t i = 0; i < fields.size(); ++i)
    {
        const auto delimiter = line.find(',');
        if (i + 1 == fields.size())
        {
            if (delimiter != std::string_view::npos) return std::nullopt;
            fields[i] = line;
        }
        else
        {
            if (delimiter == std::string_view::npos) return std::nullopt;
            fields[i] = line.substr(0, delimiter);
            line.remove_prefix(delimiter + 1);
        }
    }
    FsrRawFrame frame;
    if (fields[0] != "RF1" || !parseUnsigned(fields[1], frame.sequence)
        || !parseUnsigned(fields[2], frame.deviceMillis)) return std::nullopt;

    for (std::size_t i = 0; i < frame.adc.size(); ++i)
    {
        std::uint32_t adc{};
        if (!parseUnsigned(fields[i + 3], adc) || adc > FsrProtocol::ADC_MAX)
            return std::nullopt;
        frame.adc[i] = static_cast<int>(adc);
    }
    return frame;
}
