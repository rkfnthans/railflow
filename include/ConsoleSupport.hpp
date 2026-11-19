#pragma once

#include <string_view>

void installConsoleStopHandler();
bool consoleStopRequested() noexcept;
void writeConsoleOutput(std::string_view text);
void printCommandLineDiagnostics(int argc, char* argv[]);
