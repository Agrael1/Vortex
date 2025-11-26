#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vortex/app.h>
#include <vortex/ui/cef/app.h>
#include <vortex/util/log_storage.h>

namespace {
bool ShouldWriteLogsToFiles()
{
    if (const char* env = std::getenv("VORTEX_CONSOLE_ONLY_LOGS")) {
        std::string_view value(env);
        return !(value == "1" || value == "true" || value == "TRUE");
    }
    return true;
}
} // namespace

int EntryMain(std::span<std::string_view> args, CefRefPtr<vortex::ui::VortexCefApp> cef_app)
try {
    const bool log_to_files = ShouldWriteLogsToFiles();
    if (log_to_files) {
        std::error_code ec;
        std::filesystem::create_directories("logs", ec);
    }

    bool debug = !log_to_files;
    vortex::MainArgs parsed_args = vortex::ParseArgs(args);
    vortex::TerminalHandler::Instance();
    vortex::LogStorage log_storage;

    // Make a default log for UI / CEF
    vortex::LogOptions options_cef{ .name = vortex::ui_log_name,
                                    .pattern_prefix = "vortex.cef",
                                    .output_file_path = debug ? "" : "cef_debug.log" };
    vortex::LogView log_ui = log_storage.CreateLog(options_cef, true);

    // Initialize CEF subprocess
    int result = cef_app->StartCefSubprocess();
    if (result >= 0) {
        vortex::info("CEF subprocess exited with code: {}", result);
        return result; // This is a CEF subprocess, exit with its return code
    }

    // Create default logs after CEF initialization
    log_storage.CreateDefaultLogs(debug);

    auto init_guard = cef_app->InitializeCef();
    if (!init_guard) {
        return -1; // CEF initialization failed
    }

    // Initialize Node Library
    vortex::RegisterHardwareNodes();
    return vortex::App{ parsed_args }.Run();
} catch (const std::exception& e) {
    vortex::critical(e.what());
    return 1;
} catch (...) {
    vortex::critical("Unknown exception");
    return 2;
}

int main(int argc, char* argv[])
{
    // Better main function
    std::size_t size = argc * sizeof(std::string_view);
    std::size_t space = size + alignof(std::string_view) - 1;

    auto* raw = alloca(space);
    auto* args = static_cast<std::string_view*>(
            std::align(alignof(std::string_view), size, raw, space));
    for (int i = 0; i < argc; ++i) {
        std::construct_at(&args[i], argv[i]); // Construct std::string_view from char* without UB
    }

    CefRefPtr<vortex::ui::VortexCefApp> cef_app(new vortex::ui::VortexCefApp{ argc, argv });
    int ret = EntryMain(std::span(args, argc), std::move(cef_app));
    std::destroy_n(args, argc); // Destroy std::string_view objects
    return ret;
}