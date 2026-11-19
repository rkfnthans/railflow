#include "RuntimeOptions.hpp"

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
    std::size_t number(std::string_view text, std::size_t limit)
    {
        std::size_t value{};
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value > limit)
        {
            throw std::invalid_argument("invalid numeric option");
        }
        return value;
    }
}

RuntimeOptions parseRuntimeOptions(std::span<const std::string_view> arguments)
{
    RuntimeOptions options;
    bool demo = false;
    std::optional<std::size_t> frames;
    bool customPeriod = false;
    bool noLogSpecified = false;
    for (std::size_t i = 0; i < arguments.size(); ++i)
    {
        const auto arg = arguments[i];
        if (arg == "--demo") demo = true;
        else if (arg == "--help") options.help = true;
        else if (arg == "--layout") options.showLayout = true;
        else if (arg == "--no-log") { options.writeCsv = false; noLogSpecified = true; }
        else if (arg == "--frames" || arg == "--period-ms")
        {
            if (++i == arguments.size()) throw std::invalid_argument("missing option value");
            if (arg == "--frames") frames = number(arguments[i], 1000000);
            else
            {
                const auto period = number(arguments[i], 60000);
                if (period == 0) throw std::invalid_argument("period must be positive");
                options.loop.period = std::chrono::milliseconds{period};
                customPeriod = true;
            }
        }
        else throw std::invalid_argument("unknown option: " + std::string(arg));
    }
    if (options.showLayout && (demo || frames || customPeriod || noLogSpecified || options.help
                              || arguments.size() != 1))
    {
        throw std::invalid_argument("--layout must be used alone");
    }
    if (!demo && (frames || customPeriod))
    {
        throw std::invalid_argument("--frames and --period-ms require --demo");
    }
    options.loop.maxFrames = frames.value_or(demo ? 0 : 1);
    return options;
}
