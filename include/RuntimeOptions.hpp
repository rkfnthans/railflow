#pragma once

#include <span>
#include <string_view>
#include "UpdateLoop.hpp"

struct RuntimeOptions
{
    LoopOptions loop;
    bool writeCsv{true};
    bool help{false};
    bool showLayout{false};
};

RuntimeOptions parseRuntimeOptions(std::span<const std::string_view> arguments);
