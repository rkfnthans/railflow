#pragma once

#include <optional>
#include <string_view>
#include "FsrRawFrame.hpp"

// 한 줄 내용 또는 한 개의 LF/CRLF로 끝나는 RF1 line을 parse한다.
// 숫자는 부호/공백 없는 10진 정수만 허용. 잘못된 줄은 nullopt.
std::optional<FsrRawFrame> parseFsrSerialFrame(std::string_view line);
