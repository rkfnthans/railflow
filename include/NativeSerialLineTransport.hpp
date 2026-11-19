#pragma once

#ifdef _WIN32
#include "Win32SerialLineTransport.hpp"
using NativeSerialLineTransport = Win32SerialLineTransport;
#else
#include "PosixSerialLineTransport.hpp"
using NativeSerialLineTransport = PosixSerialLineTransport;
#endif
