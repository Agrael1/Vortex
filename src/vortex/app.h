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
        _ui_app.BindFunction("createNode", [this](CefListValue& args) { CreateNode(args); });
        _ui_app.BindFunction("greetAsync", [this](CefListValue& args) { GreetAsync(args); });
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

    void CreateNode(CefListValue& args)
    {
        if (args.GetSize() > 0 && args.GetType(0) == VTYPE_STRING) {
            std::string func_name = args.GetString(0);
            // Call the corresponding function in the model or handle it
            _model.CreateNode(_gfx);
        }
    }
    void GreetAsync(CefListValue& args2)
    {
        auto frame = _ui_app.GetClient()->GetBrowser()->GetMainFrame();

        auto a = CefProcessMessage::Create("co_return");
        auto args = a->GetArgumentList();
        args->SetSize(1); // Set size to 1 for the counter
        args->SetInt(0, counter++); // Increment the counter and send it back
        frame->SendProcessMessage(PID_BROWSER, a);
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

    uint32_t counter = 32; ///< Counter for async calls

private:
    const AppExitControl& _exit;
};
} // namespace vortex