#include <csignal>
#include <vortex/app.h>
#include <util/log.h>

struct MainArgs {
};

int entry_main(std::span<std::string_view> args)
try {
    MainArgs main_args;

    bool debug = true;
    vortex::LogOptions options{
        .name = vortex::app_log_name,
        .pattern_prefix = "vortex",
        .output_file_path = debug ? "" : "logs/vortex.log"
    };
    vortex::LogOptions options_gfx{
        .name = vortex::graphics_log_name,
        .pattern_prefix = "vortex.graphics",
        .output_file_path = debug ? "" : "logs/vortex.log"
    };
    
    vortex::Log log_global{ options };
    vortex::Log log_graphics{ options_gfx };
    log_global.SetAsDefault();

    // catch Ctrl+C
    std::signal(SIGINT, [](int) {
        vortex::critical("Ctrl+C pressed");
        vortex::AppExitControl::Exit();
    });

    // Initialize Node Library
    vortex::RegisterHardwareNodes();

    vortex::App a{};
    return a.Run();
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
    auto args = reinterpret_cast<std::string_view*>(alloca(argc * sizeof(std::string_view)));
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }

    return entry_main(std::span(args, argc));
}