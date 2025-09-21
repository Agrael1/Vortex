#pragma once
#include <vortex/util/interprocess_lock.h>
#include <spdlog/common.h>
#include <spdlog/sinks/sink.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace vortex {
class wincolor_sink : public spdlog::sinks::sink
{
public:
    wincolor_sink(void* out_handle = GetStdHandle(STD_OUTPUT_HANDLE), spdlog::color_mode mode = spdlog::color_mode::automatic);
    wincolor_sink(const wincolor_sink& other) = delete;
    wincolor_sink& operator=(const wincolor_sink& other) = delete;

public:
    // change the color for the given level
    void set_color(spdlog::level::level_enum level, std::uint16_t color);
    void log(const spdlog::details::log_msg& msg) final override;
    void flush() final override { }
    void set_pattern(const std::string& pattern) override final;
    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override final;
    void set_color_mode(spdlog::color_mode mode);

protected:
    void* out_handle_;
    // set foreground color and return the orig console attributes (for resetting later)
    std::uint16_t set_foreground_color_(std::uint16_t attribs);

    // print a range of formatted message to console
    void print_range_(const spdlog::memory_buf_t& formatted, size_t start, size_t end);

    // in case we are redirected to file (not in console mode)
    void write_to_file_(const spdlog::memory_buf_t& formatted);

    void set_color_mode_impl(spdlog::color_mode mode);

protected:
    bool should_do_colors_;
    vortex::interprocess_lock mutex_;
    std::unique_ptr<spdlog::formatter> formatter_;
    std::array<std::uint16_t, spdlog::level::n_levels> colors_;
};
} // namespace vortex
