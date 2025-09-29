#pragma once
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <system_error>

namespace vortex {
struct StatePOSIX {
    int inFd = STDIN_FILENO;
    int outFd = STDOUT_FILENO;
    termios origTerm{};
};

class TerminalInputImpl
{
    friend TerminalInputImpl CreateTerminalInputImpl(std::error_code& ec) noexcept;

public:
    TerminalInputImpl() = default;
    ~TerminalInputImpl();

public:
    size_t write_utf8(std::string_view s) const;

private:
    StatePOSIX state;
    wchar_t pending_high_surrogate = 0;
};

[[nodiscard]] TerminalInputImpl CreateTerminalInputImpl(std::error_code& ec) noexcept;
}