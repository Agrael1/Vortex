#include <vortex/util/term/log_sink.h>
#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <vortex/util/term/input.h>

vortex::wincolor_sink::wincolor_sink(void* out_handle, spdlog::color_mode mode)
    : formatter_(spdlog::details::make_unique<spdlog::pattern_formatter>())
    , out_handle_(out_handle)
{
    set_color_mode_impl(mode);
    // set level colors
    colors_[spdlog::level::trace] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // white
    colors_[spdlog::level::debug] = FOREGROUND_GREEN | FOREGROUND_BLUE; // cyan
    colors_[spdlog::level::info] = FOREGROUND_GREEN; // green
    colors_[spdlog::level::warn] =
            FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // intense yellow
    colors_[spdlog::level::err] = FOREGROUND_RED | FOREGROUND_INTENSITY; // intense red
    colors_[spdlog::level::critical] = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN |
            FOREGROUND_BLUE |
            FOREGROUND_INTENSITY; // intense white on red background
    colors_[spdlog::level::off] = 0;
}

// change the color for the given level
void vortex::wincolor_sink::wincolor_sink::set_color(spdlog::level::level_enum level,
                                                     std::uint16_t color)
{
    std::lock_guard lock(mutex_);
    colors_[static_cast<size_t>(level)] = color;
}

void vortex::wincolor_sink::log(const spdlog::details::log_msg& msg)
{
    if (out_handle_ == nullptr || out_handle_ == INVALID_HANDLE_VALUE) {
        return;
    }

    std::lock_guard lock(mutex_);
    msg.color_range_start = 0;
    msg.color_range_end = 0;
    spdlog::memory_buf_t formatted;
    formatter_->format(msg, formatted);
    if (should_do_colors_ && msg.color_range_end > msg.color_range_start) {
        TerminalHandler::Instance().StartLineUnlocked();

        // before color range
        print_range_(formatted, 0, msg.color_range_start);
        // in color range
        auto orig_attribs =
                static_cast<WORD>(set_foreground_color_(colors_[static_cast<size_t>(msg.level)]));
        print_range_(formatted, msg.color_range_start, msg.color_range_end);
        // reset to orig colors
        ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), orig_attribs);

        if (formatted.size() > msg.color_range_end) {
            auto size = static_cast<DWORD>(formatted.size() - msg.color_range_end);
            TerminalHandler::Instance().EndLineUnlocked(std::string_view(formatted.data() + msg.color_range_end, size));
        }
    } else // print without colors if color range is invalid (or color is disabled)
    {
        write_to_file_(formatted);
    }
}

void vortex::wincolor_sink::set_pattern(const std::string& pattern)
{
    std::lock_guard lock(mutex_);
    formatter_ = std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(pattern));
}

void vortex::wincolor_sink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
{
    std::lock_guard lock(mutex_);
    formatter_ = std::move(sink_formatter);
}

void vortex::wincolor_sink::set_color_mode(spdlog::color_mode mode)
{
    std::lock_guard lock(mutex_);
    set_color_mode_impl(mode);
}

void vortex::wincolor_sink::set_color_mode_impl(spdlog::color_mode mode)
{
    if (mode == spdlog::color_mode::automatic) {
        // should do colors only if out_handle_  points to actual console.
        DWORD console_mode;
        bool in_console = ::GetConsoleMode(static_cast<HANDLE>(out_handle_), &console_mode) != 0;
        should_do_colors_ = in_console;
    } else {
        should_do_colors_ = mode == spdlog::color_mode::always ? true : false;
    }
}

// set foreground color and return the orig console attributes (for resetting later)

std::uint16_t vortex::wincolor_sink::set_foreground_color_(std::uint16_t attribs)
{
    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
    if (!::GetConsoleScreenBufferInfo(static_cast<HANDLE>(out_handle_), &orig_buffer_info)) {
        // just return white if failed getting console info
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    // change only the foreground bits (lowest 4 bits)
    auto new_attribs = static_cast<WORD>(attribs) | (orig_buffer_info.wAttributes & 0xfff0);
    auto ignored =
            ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), static_cast<WORD>(new_attribs));
    (void)(ignored);
    return static_cast<std::uint16_t>(orig_buffer_info.wAttributes); // return orig attribs
}

// print a range of formatted message to console

void vortex::wincolor_sink::print_range_(const spdlog::memory_buf_t& formatted,
                                         size_t start,
                                         size_t end)
{
    if (end > start) {
        auto size = static_cast<DWORD>(end - start);
        TerminalHandler::Instance().WriteUtf8(std::string_view(formatted.data() + start, size));
    }
}

void vortex::wincolor_sink::write_to_file_(const spdlog::memory_buf_t& formatted)
{
    auto size = static_cast<DWORD>(formatted.size());
    TerminalHandler::Instance().PrintLineUnlocked(std::string_view(formatted.data(), size));
}
