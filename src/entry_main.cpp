#include <csignal>
#include <vortex/app.h>
#include <vortex/util/log.h>
#include <vortex/ui/cef_app.h>

struct MainArgs {
};

int entry_main(std::span<std::string_view> args)
try {
    bool debug = true;
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

    vortex::Log log_global{ options };
    vortex::Log log_graphics{ options_gfx };
    vortex::Log log_stream{ options_stream };
    log_global.SetAsDefault();

    // catch Ctrl+C
    std::signal(SIGINT, [](int) {
        vortex::critical("Ctrl+C pressed");
        vortex::AppExitControl::Exit();
    });

    // Initialize Cef
    CefMainArgs cef_args{ GetModuleHandleW(nullptr) };
    CefRefPtr<vortex::ui::VortexCefApp> cef_app{ new vortex::ui::VortexCefApp() };
    int code = CefExecuteProcess(cef_args, cef_app, nullptr);
    if (code >= 0) {
        vortex::info("CefExecuteProcess returned: {}", code);
        return code; // If this is a secondary process, exit immediately
    }

    // Initialize CEF settings
    CefSettings cef_settings;
    cef_settings.multi_threaded_message_loop = true; // Use single-threaded for better integration
    cef_settings.no_sandbox = true; // Disable sandbox for simplicity
    CefString(&cef_settings.cache_path).FromString((std::filesystem::current_path() / u"cef_cache").string()); // Set cache path

    if (!CefInitialize(cef_args, cef_settings, cef_app, nullptr)) {
        vortex::critical("CefInitialize failed");
        return 3; // Initialization failed
    }

    // Initialize Node Library
    vortex::RegisterHardwareNodes();    

    int result = 0;
    {
        vortex::App app;
        result = app.Run();
    }

    // Shutdown CEF
    CefShutdown();

    return result;
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
    auto* args = static_cast<std::string_view*>(std::align(alignof(std::string_view), size, raw, space));
    for (int i = 0; i < argc; ++i) {
        std::construct_at(&args[i], argv[i]); // Construct std::string_view from char* without UB
    }

    int ret = entry_main(std::span(args, argc));
    std::destroy_n(args, argc); // Destroy std::string_view objects
    return ret;
}