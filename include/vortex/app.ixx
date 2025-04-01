module;
#include <stdexcept>
#include <format>
export module vortex.app;

import vortex.graphics;
import vortex.queue;
import vortex.window;

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
        : _window("Vortex", 1280, 720, false)
        , _gfx(nullptr, true)
        , _exit(AppExitControl::GetInstance())
    {
        CreateQueues();
    }

public:
    int Run()
    {
        while (!_window.PollEvents()) {
            if (_exit.exit) {
                return 1; // exit requested
            }
        }
        return 0;
    }

private:
    void CreateQueues()
    {
        // Create the render queue
        auto cluster = vortex::CreateRenderQueue(_gfx);
        if (!cluster) {
            throw std::runtime_error{ std::format("Creation of the render cluster failed: {}", cluster.error()) };
        }
        _render = std::move(cluster.value());

        // Create the copy queue
        auto copy = vortex::CreateCopyQueue(_gfx);
        if (!copy) {
            throw std::runtime_error{ std::format("Creation of the copy queue failed: {}", copy.error()) };
        }
        _copy = std::move(copy.value());
    }

private:
    vortex::Window _window; // here for now
    vortex::Graphics _gfx;

    vortex::Queue _render;
    vortex::Queue _copy;

private:
    const AppExitControl& _exit;
};
} // namespace vortex