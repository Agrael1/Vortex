#ifdef _WIN32
#include <vortex/util/term/impl.win32.h>
#include <memory>

vortex::detail::TerminalInputImpl::TerminalInputImpl(std::error_code& ec, TermKeyHandler key_handler, void* user_data)
    : key_handler(key_handler), user_data(user_data)
{
    state.h_in = GetStdHandle(STD_INPUT_HANDLE);
    if (state.h_in == INVALID_HANDLE_VALUE) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return;
    }
    state.h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (state.h_out == INVALID_HANDLE_VALUE) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return;
    }
    if (!GetConsoleMode(state.h_in, &state.orig_in_mode)) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return;
    }
    if (!GetConsoleMode(state.h_out, &state.orig_out_mode)) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return;
    }

    // Enable VT processing on output
    DWORD out_mode = state.orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(state.h_out, out_mode)) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return;
    }

    // Set raw input mode
    DWORD in_mode = state.orig_in_mode;
    in_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    in_mode &= ~ENABLE_PROCESSED_INPUT; // let Ctrl+C be a key event
    in_mode |= ENABLE_EXTENDED_FLAGS;
    in_mode &= ~ENABLE_QUICK_EDIT_MODE;
    if (!SetConsoleMode(state.h_in, in_mode)) {
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        // Try to restore output mode
        SetConsoleMode(state.h_out, state.orig_out_mode);
        return;
    }
    ec.clear();
}

vortex::detail::TerminalInputImpl::~TerminalInputImpl()
{
    if (state.orig_in_mode) {
        SetConsoleMode(state.h_in, state.orig_in_mode);
    }
    if (state.orig_out_mode) {
        SetConsoleMode(state.h_out, state.orig_out_mode);
    }
}

size_t vortex::detail::TerminalInputImpl::WriteUtf8(std::string_view s) const
{
    if (s.empty()) {
        return 0;
    }
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (wlen <= 0) {
        return 0;
    }

    std::wstring w(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), wlen);
    DWORD written = 0;
    WriteConsoleW(state.h_out, w.c_str(), static_cast<DWORD>(w.size()), &written, nullptr);
    return static_cast<size_t>(written);
}

void vortex::detail::TerminalInputImpl::PollInput()
{
    DWORD events = 0;
    if (!GetNumberOfConsoleInputEvents(state.h_in, &events) || events == 0) {
        return;
    }

    std::unique_ptr<INPUT_RECORD[]> records_buf{ new (std::nothrow) INPUT_RECORD[events] };
    if (!records_buf) {
        return;
    }

    DWORD read = 0;
    if (!ReadConsoleInputW(state.h_in, records_buf.get(), events, &read) || read == 0) {
        return;
    }
    for (DWORD i = 0; i < read; ++i) {
        const INPUT_RECORD& rec = records_buf[i];
        switch (rec.EventType) {
        case KEY_EVENT:
            HandleKeyEvent(rec.Event.KeyEvent);
            break;
        case WINDOW_BUFFER_SIZE_EVENT:
            key_handler(TermKey::Redraw, 0, user_data);
            break;
        default:
            break;
        }
    }
}

void vortex::detail::TerminalInputImpl::HandleKeyEvent(const KEY_EVENT_RECORD& kev)
{
    if (!kev.bKeyDown) {
        return;
    }

    const WORD vk = kev.wVirtualKeyCode;
    const WCHAR wch = kev.uChar.UnicodeChar;

    switch (vk) {
    case VK_LEFT:
        key_handler(TermKey::Left, 0, user_data);
        break;
    case VK_RIGHT:
        key_handler(TermKey::Right, 0, user_data);
        break;
    case VK_HOME:
        key_handler(TermKey::Home, 0, user_data);
        break;
    case VK_END:
        key_handler(TermKey::End, 0, user_data);
        break;
    case VK_UP:
        key_handler(TermKey::Up, 0, user_data);
        break; // redraw done
    case VK_DOWN:
        key_handler(TermKey::Down, 0, user_data);
        break; // redraw done
    case VK_BACK:
        key_handler(TermKey::Backspace, 0, user_data);
        break;
    case VK_DELETE:
        key_handler(TermKey::Delete, 0, user_data);
        break;
    case VK_RETURN:
        key_handler(TermKey::Enter, 0, user_data);
        break;
    default: {
        // Printable char(s). Handle surrogate pairs.
        if (wch >= 0x20 || wch == L'\t') {
            if (wch >= 0xD800 && wch <= 0xDBFF) {
                // High surrogate
                pending_high_surrogate = wch;
            } else if (wch >= 0xDC00 && wch <= 0xDFFF) {
                // Low surrogate
                if (pending_high_surrogate) {
                    wchar_t hs = pending_high_surrogate;
                    pending_high_surrogate = 0;
                    char32_t cp = 0x10000 + ((((hs - 0xD800) & 0x3FF) << 10) | ((wch - 0xDC00) & 0x3FF));
                    key_handler(TermKey::Char, cp, user_data);
                }
            } else {
                pending_high_surrogate = 0;
                key_handler(TermKey::Char, static_cast<char32_t>(wch), user_data);
            }
        }
        break;
    }
    }
}
#endif