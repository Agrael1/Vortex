#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <system_error>
#include <vortex/util/term/shared.h>

namespace vortex::detail {
struct StateWin32 {
    HANDLE h_in;
    HANDLE h_out;
    DWORD orig_in_mode = 0;
    DWORD orig_out_mode = 0;
};

class TerminalInputImpl
{
public:
    TerminalInputImpl(std::error_code& ec, TermKeyHandler key_handler, void* user_data);
    ~TerminalInputImpl();

public:
    size_t WriteUtf8(std::string_view s) const;
    void PollInput();

private:
    void HandleKeyEvent(const KEY_EVENT_RECORD& kev);

private:
    StateWin32 state;
    TermKeyHandler key_handler = nullptr;
    void* user_data = nullptr;
    wchar_t pending_high_surrogate = 0;
};
} // namespace vortex::detail