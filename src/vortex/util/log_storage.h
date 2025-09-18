#pragma once
#include <vortex/util/log.h>

namespace vortex {
class LogStorage
{
public:
    ~LogStorage() {
        logs.clear();
        spdlog::shutdown();
    }

public:
    static vortex::LogView CreateLog(const LogOptions& options, bool set_default = false)
    {
        auto& log = logs.emplace_back(options);
        if (set_default) {
            log.SetAsDefault();
        }
        return log;
    }
    static LogView GetLog(std::string_view name)
    {
        return LogView{ spdlog::get(name.data()) };
    }

    static LogView operator()(std::string_view name)
    {
        return GetLog(name);
    }

    static void CreateDefaultLogs(bool debug)
    {
        vortex::LogOptions options{
            .name = vortex::app_log_name,
            .pattern_prefix = "vortex",
            .output_file_path = debug ? "" : "logs/vortex.log"
        };
        vortex::LogOptions options_gfx{
            .name = vortex::graphics_log_name,
            .pattern_prefix = "vortex.graphics",
            .output_file_path = debug ? "" : "logs/vortex.gfx.log"
        };
        vortex::LogOptions options_stream{
            .name = vortex::stream_log_name,
            .pattern_prefix = "vortex.stream",
            .output_file_path = debug ? "" : "logs/vortex.stream.log"
        };

        CreateLog(options, true);
        CreateLog(options_gfx);
        CreateLog(options_stream);
    }

private:
    static inline std::vector<Log> logs;
};
} // namespace vortex