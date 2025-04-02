module;
#include <stdexcept>
#include <format>
export module vortex.app;

import vortex.graphics;
import vortex.sdl;
import vortex.swapchain;

namespace vortex {
export struct AppExitControl {

    AppExitControl() = default;
    AppExitControl(const AppExitControl&) = delete;

    static void Exit()
    {
        GetInstance().exit = true;
    }

    static AppExitControl& GetInstance()
    {
        static AppExitControl instance;
        return instance;
    }

    bool exit = false;
};
export class App
{
public:
    App()
        : _gfx(true)
        , _window(_gfx, "Vortex", 1280, 720)
        , _exit(AppExitControl::GetInstance())
    {
    }

public:
    int Run()
    {
        while (!_window.ProcessEvents()) {
            if (_exit.exit) {
                return 1; // exit requested
            }
        }
        return 0;
    }

private:
    vortex::SDLInstance _sdl;
    vortex::Graphics _gfx;
    vortex::DebugOutput _window;

private:
    const AppExitControl& _exit;
};
} // namespace vortex