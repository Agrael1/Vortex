#pragma once
#include <vortex/util/interprocess_lock.h>
#include <vector>

#ifdef _WIN32
#include <vortex/util/term/impl.win32.h>
#else
#include <vortex/util/term/impl.posix.h>
#endif

namespace vortex {
using TermInputHandler = bool (*)(std::string_view line, void* user_data);

class TerminalInput
{
    static constexpr size_t max_history = 1000;
    static constexpr std::string_view prompt = "> ";

public:
    TerminalInput(std::error_code& ec) noexcept
        : impl(ec, [](TermKey key, char32_t ch, void* p) { static_cast<TerminalInput*>(p)->HandleKeyEvent(key, ch); }, this)
    {
    }

public:
    void SetInputHandler(TermInputHandler handler, void* user_data = nullptr);
    size_t WriteUtf8(std::string_view s) const
    {
        return impl.WriteUtf8(s);
    }
    void PollInput()
    {
        impl.PollInput();
    }
    void PrintLine(std::string_view msg);
    void PrintLineUnlocked(std::string_view msg);
    void StartLineUnlocked();
    void EndLineUnlocked(std::string_view msg);
    void ClearOutput();

private:
    void HandleKeyEvent(TermKey key, char32_t ch);
    void RedrawInput();
    void RedrawInputUnlocked();

    // History management
    void AddToHistory(std::string_view line);
    void HistoryStartIfNeeded();
    void HistoryUp();
    void HistoryDown();
    void AbortHistoryBrowseOnEdit();

private:
    detail::TerminalInputImpl impl;
    std::string input; // UTF-8 input buffer
    size_t cursor = 0; // byte offset at a code-point boundary

    // History
    std::vector<std::string> history;
    size_t hist_index : sizeof(size_t) * CHAR_BIT - 1 = 0; // index while browsing history
    size_t hist_index_valid : 1 = 0; // whether hist_index is valid
    std::string saved_before_history; // line saved before entering history mode

    vortex::interprocess_lock io_mutex;

    // Input callback
    TermInputHandler _line_handler = nullptr;
    void* _user_data = nullptr;
};

class TerminalHandler
{
public:
    static TerminalInput& Instance() noexcept
    {
        static std::error_code ec;
        static TerminalInput instance(ec);
        return instance;
    }
};

static void PrintLine(std::string_view msg)
{
    TerminalHandler::Instance().PrintLine(msg);
}
static void PrintLineUnlocked(std::string_view msg)
{
    TerminalHandler::Instance().PrintLineUnlocked(msg);
}
static void PollTerminalInput()
{
    TerminalHandler::Instance().PollInput();
}

} // namespace vortex