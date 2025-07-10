#pragma once
#include <vector>
#include <memory>
#include <vortex/graphics.h>
#include <vortex/node_registry.h>
#include <vortex/pipeline_storage.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/ui/ui_app.h>
#include <vortex/model.h>

namespace vortex {
struct AppExitControl {

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

class App
{
public:
    App()
        : _gfx(true)
        , _exit(AppExitControl::GetInstance())
        , _ui_app("Vortex Application", 1920, 1080, false)
        , _descriptor_buffer(_gfx)
    {
    }

public:
    int Run()
    {
        while (!_exit.exit) {
            if (int code = _ui_app.ProcessEvents()) {
                return code; // Exit requested
            }
        }

        return 0;
    }

private:
    vortex::NDILibrary _ndi;
    vortex::ui::SDLLibrary _sdl;

    vortex::Graphics _gfx;
    vortex::PipelineStorage _pipeline_storage;
    vortex::DescriptorBuffer _descriptor_buffer;

    // CEF client for UI
    vortex::ui::UIApp _ui_app;


    vortex::GraphModel _model; ///< Model containing nodes and outputs

private:
    const AppExitControl& _exit;
};
} // namespace vortex