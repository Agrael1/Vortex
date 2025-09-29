#include <vortex/util/term/input.h>
#include <utf8.h>
#include <mutex>

static size_t prev_cp(const std::string& s, size_t pos)
{
    if (pos == 0) {
        return 0;
    }
    auto it = s.begin() + static_cast<std::ptrdiff_t>(pos);
    try {
        utf8::prior(it, s.begin());
    } catch (...) {
        // If invalid, fall back one byte safely
        --it;
        while (it != s.begin() && ((*it & 0xC0) == 0x80)) {
            --it;
        }
    }
    return static_cast<size_t>(it - s.begin());
}

static size_t next_cp(const std::string& s, size_t pos)
{
    if (pos >= s.size()) {
        return s.size();
    }
    auto it = s.begin() + static_cast<std::ptrdiff_t>(pos);
    try {
        utf8::next(it, s.end());
    } catch (...) {
        // If invalid, skip to next non-continuation byte
        ++it;
        while (it != s.end() && ((*it & 0xC0) == 0x80)) {
            ++it;
        }
    }
    return static_cast<size_t>(it - s.begin());
}

static size_t count_cps_right(const std::string& s, size_t pos)
{
    auto it = s.begin() + static_cast<std::ptrdiff_t>(pos);
    try {
        return static_cast<size_t>(utf8::distance(it, s.end()));
    } catch (...) {
        // If invalid, approximate by counting non-continuation bytes
        size_t count = 0;
        for (auto i = it; i != s.end(); ++i) {
            if (((*i) & 0xC0) != 0x80) {
                ++count;
            }
        }
        return count;
    }
}

static void insert_cp_utf8(std::string& s, size_t pos, char32_t cp)
{
    std::string encoded;
    try {
        utf8::append(cp, std::back_inserter(encoded));
    } catch (...) {
        return; // ignore invalid code points
    }
    s.insert(s.begin() + static_cast<std::ptrdiff_t>(pos), encoded.begin(), encoded.end());
}

static size_t erase_prev_cp(std::string& s, size_t cursorByte)
{
    if (cursorByte == 0) {
        return 0;
    }
    size_t p = prev_cp(s, cursorByte);
    s.erase(s.begin() + static_cast<std::ptrdiff_t>(p),
            s.begin() + static_cast<std::ptrdiff_t>(cursorByte));
    return p;
}

static void erase_next_cp(std::string& s, size_t cursorByte)
{
    if (cursorByte >= s.size()) {
        return;
    }
    size_t n = next_cp(s, cursorByte);
    s.erase(s.begin() + static_cast<std::ptrdiff_t>(cursorByte),
            s.begin() + static_cast<std::ptrdiff_t>(n));
}

void vortex::TerminalInput::SetInputHandler(TermInputHandler handler, void* user_data)
{
    std::lock_guard lock(io_mutex);
    _line_handler = handler;
    _user_data = user_data;
}

void vortex::TerminalInput::PrintLine(std::string_view msg)
{
    std::lock_guard lock(io_mutex);
    PrintLineUnlocked(msg);
}

void vortex::TerminalInput::PrintLineUnlocked(std::string_view msg)
{
    // Clear current line
    WriteUtf8("\r\x1b[0K");
    // Print message
    WriteUtf8(msg);
    if (msg.empty() || msg.back() != '\n') {
        WriteUtf8("\n");
    }
    RedrawInputUnlocked();
}

void vortex::TerminalInput::StartLineUnlocked()
{
    WriteUtf8("\r\x1b[0K");
}

void vortex::TerminalInput::EndLineUnlocked(std::string_view msg)
{
    // Print message
    WriteUtf8(msg);
    if (msg.empty() || msg.back() != '\n') {
        WriteUtf8("\n");
    }
    RedrawInputUnlocked();
}

void vortex::TerminalInput::ClearOutput()
{
    std::lock_guard lock(io_mutex);
    WriteUtf8("\x1b[2J\x1b[H");
    RedrawInputUnlocked();
}

void vortex::TerminalInput::HandleKeyEvent(TermKey key, char32_t ch)
{
    switch (key) {
    case vortex::TermKey::Left:
        cursor = prev_cp(input, cursor);
        break;
    case vortex::TermKey::Right:
        cursor = next_cp(input, cursor);
        break;
    case vortex::TermKey::Home:
        cursor = 0;
        break;
    case vortex::TermKey::End:
        cursor = input.size();
        break;
    case vortex::TermKey::Up:
        HistoryUp();
        return; // redraw done
    case vortex::TermKey::Down:
        HistoryDown();
        return; // redraw done
    case vortex::TermKey::Delete:
        AbortHistoryBrowseOnEdit();
        break;
    case vortex::TermKey::Enter:
        AddToHistory(input);
        cursor = 0;
        hist_index_valid = 0;
        saved_before_history.clear();
        if (!input.empty()) {
            WriteUtf8("\r\x1b[0K");
            WriteUtf8(std::format("{}{}\n", prompt, input));

            if (input == "clear" || input == "cls") {
                ClearOutput();
            } else if (_line_handler) {
                bool handled = _line_handler(input, _user_data);
                if (!handled) {
                    PrintLineUnlocked("Unrecognized command: " + input);
                }
            }

        } else {
            WriteUtf8("\r\x1b[0K\n");
        }
        input.clear();
        break;
    case vortex::TermKey::Backspace:
        AbortHistoryBrowseOnEdit();
        cursor = erase_prev_cp(input, cursor);
        break;
    case vortex::TermKey::Char:
        AbortHistoryBrowseOnEdit();
        insert_cp_utf8(input, cursor, ch);
        cursor = next_cp(input, cursor);
        break;
    default:
        break;
    }
    RedrawInput();
}

void vortex::TerminalInput::RedrawInput() 
{
    std::lock_guard lock(io_mutex);
    RedrawInputUnlocked();
}
void vortex::TerminalInput::RedrawInputUnlocked()
{
    // Clear line: CR + CSI 0K
    WriteUtf8(std::format("\r\x1b[0K {}{}", prompt, input));

    // Move caret left by code points to the right of cursor
    size_t cpsRight = count_cps_right(input, cursor);
    if (cpsRight > 0) {
        WriteUtf8(std::format("\x1b[{}D", cpsRight));
    }
}

// History management
void vortex::TerminalInput::AddToHistory(std::string_view line)
{
    if (line.empty()) {
        return;
    }
    if (!history.empty() && history.back() == line) {
        return;
    }
    history.emplace_back(line.data(), line.size());
    if (history.size() > max_history) {
        history.erase(history.begin());
    }
}

void vortex::TerminalInput::HistoryStartIfNeeded()
{
    if (hist_index_valid || history.empty()) {
        return;
    }

    saved_before_history = input;
    hist_index = history.size() - 1;
    hist_index_valid = 1;
    input = history[hist_index];
    cursor = input.size();
    RedrawInput();
}

void vortex::TerminalInput::HistoryUp()
{
    if (history.empty()) {
        return;
    }
    if (!hist_index_valid) {
        HistoryStartIfNeeded();
        return;
    }
    if (hist_index > 0) {
        hist_index--;
        input = history[hist_index];
        cursor = input.size();
        RedrawInput();
    }
}

void vortex::TerminalInput::HistoryDown()
{
    if (!hist_index_valid) {
        return;
    }
    if (hist_index + 1 < history.size()) {
        hist_index++;
        input = history[hist_index];
        cursor = input.size();
    } else {
        input = saved_before_history;
        cursor = input.size();
        hist_index_valid = 0;
        saved_before_history.clear();
    }
    RedrawInput();
}

void vortex::TerminalInput::AbortHistoryBrowseOnEdit()
{
    if (hist_index_valid) {
        hist_index_valid = 0;
        saved_before_history.clear();
    }
}
