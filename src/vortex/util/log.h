#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <format>
#include <vortex/util/interprocess_lock.h>

namespace vortex {
struct console_interprocess_mutex {
    using mutex_t = vortex::interprocess_lock;
    static mutex_t& mutex()
    {
        static mutex_t s_mutex;
        return s_mutex;
    }
};
#ifdef _WIN32
using cross_process_console_sink = spdlog::sinks::wincolor_stdout_sink<console_interprocess_mutex>;
#else
using cross_process_console_sink = spdlog::sinks::ansicolor_stdout_sink<console_interprocess_mutex>;
#endif

struct LogOptions {
    std::string_view name;
    std::string_view pattern_prefix;
    std::filesystem::path output_file_path;
};
class LogView
{
public:
    LogView() = default;
    LogView(const std::shared_ptr<spdlog::logger>& logger)
        : logger(logger)
    {
    }

public:
    operator bool() const
    {
        return logger != nullptr;
    }

    void info(std::string_view message) const
    {
        logger->info(message);
    }
    void warn(std::string_view message) const
    {
        logger->warn(message);
    }
    void error(std::string_view message) const
    {
        logger->error(message);
    }
    void critical(std::string_view message) const
    {
        logger->critical(message);
    }
    void trace(std::string_view message) const
    {
        logger->trace(message);
    }
    void debug(std::string_view message) const
    {
        logger->debug(message);
    }

    template<class... _Types>
    void info(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        info(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void warn(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        warn(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void error(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        error(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void critical(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        critical(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void trace(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        trace(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void debug(const std::format_string<_Types...> fmt, _Types&&... args) const
    {
        debug(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

private:
    std::shared_ptr<spdlog::logger> logger;
};

class Log
{
public:
    Log(const LogOptions& options)
    {
        sinks[0] = std::make_shared<cross_process_console_sink>();
        sinks[1] = !options.output_file_path.empty()
                ? std::make_shared<spdlog::sinks::basic_file_sink_st>(options.output_file_path.string(), true)
                : nullptr;
        std::string pattern = std::format("[{}] [%^%l%$] %v", options.pattern_prefix);
        bool to_file = sinks[1] != nullptr;
        auto& console_sink = *std::get<0>(sinks);
        console_sink.set_level(spdlog::level::trace);
        console_sink.set_pattern(pattern);

        if (to_file) {
            auto& file_sink = *std::get<1>(sinks);
            file_sink.set_level(spdlog::level::warn);
            file_sink.set_pattern(pattern);
        }
        logger = std::make_shared<spdlog::logger>(std::string(options.name), sinks.begin(), sinks.end() - !to_file);
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }
    operator LogView()
    {
        return LogView{ logger };
    }
    void SetAsDefault()
    {
        spdlog::set_default_logger(logger);
    }

private:
    std::array<spdlog::sink_ptr, 2> sinks;
    std::shared_ptr<spdlog::logger> logger;
};

inline void info(std::string_view message)
{
    spdlog::info(message);
}
inline void warn(std::string_view message)
{
    spdlog::warn(message);
}
inline void error(std::string_view message)
{
    spdlog::error(message);
}
inline void critical(std::string_view message)
{
    spdlog::critical(message);
}
inline void trace(std::string_view message)
{
    spdlog::trace(message);
}

inline void debug(std::string_view message)
{
    spdlog::debug(message);
}

// Template functions for formatted logging
template<class... _Types>
void info(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::info(std::vformat(fmt.get(), std::make_format_args(args...)));
}
template<class... _Types>
void warn(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::warn(std::vformat(fmt.get(), std::make_format_args(args...)));
}
template<class... _Types>
void error(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::error(std::vformat(fmt.get(), std::make_format_args(args...)));
}
template<class... _Types>
void critical(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::critical(std::vformat(fmt.get(), std::make_format_args(args...)));
}
template<class... _Types>
void trace(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::trace(std::vformat(fmt.get(), std::make_format_args(args...)));
}
template<class... _Types>
void debug(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::debug(std::vformat(fmt.get(), std::make_format_args(args...)));
}

constexpr inline std::string_view graphics_log_name = "vortex.graphics";
constexpr inline std::string_view stream_log_name = "vortex.stream";
constexpr inline std::string_view ui_log_name = "vortex.ui";
constexpr inline std::string_view app_log_name = "vortex";
} // namespace vortex