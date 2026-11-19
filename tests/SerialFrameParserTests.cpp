#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "SerialFrameParser.hpp"

int main()
{
    int passed = 0;
    int total = 0;
    const auto check = [&](bool result, const std::string& name)
    {
        ++total;
        passed += result;
        std::cout << (result ? "[PASS] " : "[FAIL] ") << name << '\n';
    };
    struct ValidCase
    {
        std::string_view line;
        std::uint64_t sequence;
        std::uint32_t millis;
        std::array<int, 9> adc;
    };
    const std::array<ValidCase, 4> valid{{
        {"RF1,0,0,0,0,0,0,0,0,0,0,0", 0, 0, {}},
        {"RF1,42,15320,231,228,235,410,398,405,120,118,121", 42, 15320,
         {231, 228, 235, 410, 398, 405, 120, 118, 121}},
        {"RF1,999999,4294967295,1023,1023,1023,1023,1023,1023,1023,1023,1023", 999999,
         std::numeric_limits<std::uint32_t>::max(),
         {1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023}},
        {"RF1,18446744073709551615,4294967295,1023,1023,1023,1023,1023,1023,1023,1023,1023",
         std::numeric_limits<std::uint64_t>::max(), std::numeric_limits<std::uint32_t>::max(),
         {1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023}}
    }};
    for (const auto& test : valid)
    {
        for (const std::string_view ending : {"", "\n", "\r\n"})
        {
            const auto frame = parseFsrSerialFrame(std::string{test.line} + std::string{ending});
            check(frame && frame->sequence == test.sequence && frame->deviceMillis == test.millis
                  && frame->adc == test.adc, "valid frame, channel order and line endings");
        }
    }
    const std::string line = "RF1,42,15320,231,228,235,410,398,405,120,118,121";
    const std::vector<std::pair<std::string, std::string>> invalid{
        {"empty", ""},
        {"version", "RF2,42,15320,231,228,235,410,398,405,120,118,121"},
        {"missing prefix", line.substr(4)},
        {"missing field", "RF1,42,15320,231,228,235,410,398,405,120,118"},
        {"extra field", line + ",1"},
        {"extra trailing token", line + ",garbage"},
        {"trailing delimiter", line + ","},
        {"empty ADC field", "RF1,42,15320,231,,235,410,398,405,120,118,121"},
        {"wrong delimiter", "RF1;42,15320,231,228,235,410,398,405,120,118,121"},
        {"leading whitespace", " " + line},
        {"trailing whitespace", line + " "},
        {"trailing garbage", line + "junk"},
        {"bare CR", line + "\r"},
        {"multiple LF", line + "\n\n"},
        {"embedded newline", "RF1,42,15320,231,228,235,410,398,405,120,118,\n121"},
        {"multiple frames", line + "\n" + line + "\n"},
        {"embedded NUL", line + std::string(1, '\0')},
        {"overlong line", std::string(FsrProtocol::MAX_LINE_CONTENT_BYTES + 3, '0')}
    };
    for (const auto& [name, text] : invalid) check(!parseFsrSerialFrame(text), name);

    for (const std::string_view value : {"-1", "-0", "+1", "1024", "4294967296", "abc",
                                        "1.0", "NaN", "inf", "1e2", "0x10", " 1", "1 "})
    {
        check(!parseFsrSerialFrame("RF1,42,15320," + std::string{value}
              + ",228,235,410,398,405,120,118,121"), "invalid ADC: " + std::string{value});
    }
    for (const std::string_view value : {"", "-1", "+1", "abc", "1.5", "18446744073709551616"})
        check(!parseFsrSerialFrame("RF1," + std::string{value}
              + ",15320,231,228,235,410,398,405,120,118,121"), "invalid sequence");
    for (const std::string_view value : {"", "-1", "+1", "abc", "1.5", "4294967296"})
        check(!parseFsrSerialFrame("RF1,42," + std::string{value}
              + ",231,228,235,410,398,405,120,118,121"), "invalid deviceMillis");

    for (std::size_t channel = 0; channel < FsrProtocol::CHANNEL_COUNT; ++channel)
    {
        std::string text = "RF1,0,0";
        for (std::size_t i = 0; i < FsrProtocol::CHANNEL_COUNT; ++i)
            text += i == channel ? ",1024" : ",0";
        check(!parseFsrSerialFrame(text), "ADC range checked on every channel");
    }
    std::cout << "SerialFrameParser checks: " << passed << '/' << total << '\n';
    return passed == total ? 0 : 1;
}
