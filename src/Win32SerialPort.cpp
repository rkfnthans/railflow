#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <system_error>
#include "Win32SerialPort.hpp"

namespace
{
    [[noreturn]] void portError(const char* operation)
    {
        throw std::system_error(static_cast<int>(::GetLastError()), std::system_category(), operation);
    }

    class ComPort final : public Win32SerialPort
    {
    public:
        explicit ComPort(const std::wstring& path)
            : handle_(::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                    nullptr, OPEN_EXISTING, 0, nullptr))
        {
            if (handle_ == INVALID_HANDLE_VALUE) portError("Open COM port (verify port number and exclusive access)");
        }

        ~ComPort() override { ::CloseHandle(handle_); }

        void configure(unsigned int baudRate)
        {
            DCB settings{};
            settings.DCBlength = sizeof(settings);
            if (!::GetCommState(handle_, &settings)) portError("GetCommState");
            settings.BaudRate = baudRate;
            settings.ByteSize = 8;
            settings.Parity = NOPARITY;
            settings.StopBits = ONESTOPBIT;
            settings.fBinary = TRUE;
            settings.fParity = FALSE;
            settings.fOutxCtsFlow = FALSE;
            settings.fOutxDsrFlow = FALSE;
            settings.fDsrSensitivity = FALSE;
            settings.fDtrControl = DTR_CONTROL_ENABLE;
            settings.fRtsControl = RTS_CONTROL_DISABLE;
            settings.fOutX = FALSE;
            settings.fInX = FALSE;
            settings.fNull = FALSE;
            settings.fErrorChar = FALSE;
            settings.fAbortOnError = FALSE;
            if (!::SetCommState(handle_, &settings)) portError("SetCommState (8N1)");
        }

        Win32SerialRead read(std::span<char> buffer, std::chrono::milliseconds timeout) override
        {
            const auto waitMs = static_cast<DWORD>(timeout.count());
            if (waitMs != configuredWaitMs_)
            {
                // Return queued bytes immediately, otherwise wait for the first byte or deadline.
                COMMTIMEOUTS timeouts{};
                timeouts.ReadIntervalTimeout = MAXDWORD;
                timeouts.ReadTotalTimeoutMultiplier = waitMs == 0 ? 0 : MAXDWORD;
                timeouts.ReadTotalTimeoutConstant = waitMs;
                if (!::SetCommTimeouts(handle_, &timeouts)) portError("SetCommTimeouts (check COM connection)");
                configuredWaitMs_ = waitMs;
            }
            DWORD size{};
            if (!::ReadFile(handle_, buffer.data(), static_cast<DWORD>(buffer.size()), &size, nullptr))
                portError("Read COM port (check connection and restart capture)");

            DWORD errors{};
            if (!::ClearCommError(handle_, &errors, nullptr)) portError("ClearCommError (check COM connection)");
            if (errors & (CE_RXOVER | CE_OVERRUN | CE_RXPARITY | CE_FRAME | CE_BREAK))
            {
                if (!::PurgeComm(handle_, PURGE_RXCLEAR)) portError("PurgeComm (check COM connection)");
                return {0, true};
            }
            return {size, false};
        }

    private:
        HANDLE handle_;
        DWORD configuredWaitMs_{MAXDWORD};
    };
}

std::unique_ptr<Win32SerialPort> openWin32SerialPort(const std::wstring& path, unsigned int baudRate)
{
    auto port = std::make_unique<ComPort>(path);
    port->configure(baudRate);
    return port;
}
