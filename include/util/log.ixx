module;
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>
#include <format>
export module vortex.log;

namespace vortex {
export struct LogOptions {
    std::string_view name;
    std::string_view pattern_prefix;
    std::filesystem::path output_file_path;
};
export class LogView
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

    void info(std::string_view message)
    {
        logger->info(message);
    }
    void warn(std::string_view message)
    {
        logger->warn(message);
    }
    void error(std::string_view message)
    {
        logger->error(message);
    }
    void critical(std::string_view message)
    {
        logger->critical(message);
    }
    void trace(std::string_view message)
    {
        logger->trace(message);
    }
    void debug(std::string_view message)
    {
        logger->debug(message);
    }

    template<class... _Types>
    void info(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        info(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void warn(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        warn(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void error(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        error(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void critical(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        critical(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void trace(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        trace(std::vformat(fmt.get(), std::make_format_args(args...)));
    }
    template<class... _Types>
    void debug(const std::format_string<_Types...> fmt, _Types&&... args)
    {
        debug(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

private:
    std::shared_ptr<spdlog::logger> logger;
};

export class Log
{
public:
    Log(const LogOptions& options)
    {
        sinks[0] = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
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

export inline Log CreateLog(const LogOptions& options, bool set_default = false)
{
    Log log{ options };
    if (set_default) {
        log.SetAsDefault();
    }
    return log;
}

export void info(std::string_view message)
{
    spdlog::info(message);
}
export void warn(std::string_view message)
{
    spdlog::warn(message);
}
export void error(std::string_view message)
{
    spdlog::error(message);
}
export void critical(std::string_view message)
{
    spdlog::critical(message);
}
export void trace(std::string_view message)
{
    spdlog::trace(message);
}

export void debug(std::string_view message)
{
    spdlog::debug(message);
}

// Template functions for formatted logging
export template<class... _Types>
void info(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::info(std::vformat(fmt.get(), std::make_format_args(args...)));
}
export template<class... _Types>
void warn(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::warn(std::vformat(fmt.get(), std::make_format_args(args...)));
}
export template<class... _Types>
void error(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::error(std::vformat(fmt.get(), std::make_format_args(args...)));
}
export template<class... _Types>
void critical(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::critical(std::vformat(fmt.get(), std::make_format_args(args...)));
}
export template<class... _Types>
void trace(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::trace(std::vformat(fmt.get(), std::make_format_args(args...)));
}
export template<class... _Types>
void debug(const std::format_string<_Types...> fmt, _Types&&... args)
{
    spdlog::debug(std::vformat(fmt.get(), std::make_format_args(args...)));
}

export LogView GetLog(std::string_view name)
{
    return LogView{ spdlog::get(name.data()) };
}

export constexpr inline std::string_view graphics_log_name = "vortex.graphics";
export constexpr inline std::string_view app_log_name = "vortex";
} // namespace vortex