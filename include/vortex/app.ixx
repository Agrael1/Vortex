module;
#include <stdexcept>
#include <format>
export module vortex.app;

import vortex.graphics;
import vortex.sdl;
import vortex.ndi_output;

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
        , _ndi_output("Vortex")
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
    vortex::NDILibrary _ndi;
    vortex::SDLLibrary _sdl;

    vortex::Graphics _gfx;

    vortex::WindowOutput _window;
    vortex::NDIOutput _ndi_output;

private:
    const AppExitControl& _exit;
};
} // namespace vortex