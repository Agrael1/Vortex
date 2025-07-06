#pragma once
#include <vector>
#include <memory>
#include <vortex/graphics.h>
#include <vortex/node_registry.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/pipeline_storage.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/ui/ui_app.h>

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
        _outputs.reserve(2);

        //codec::CodecFFmpeg::LoadTexture(_gfx, "C:/Users/Agrae/Downloads/HDR.jpg");
        _image_input = std::make_unique<vortex::ImageInput>(
            _gfx,
            vortex::ImageInput::ImageParams{
                .data = {
                    .image_path = "C:/Users/Agrae/Downloads/HDR.jpg",
                    .crop_rect = { 0.f, 0.f, 1.f, 1.f }, // Full image
                    .size = { 1280, 720 },
                    .origin = { 0, 0 },
                    .rotation = { 0.f, 0.f }
                } });
        //vortex::OutputDesc window{
        //    .format = wis::DataFormat::RGBA8Unorm,
        //    .size = { 1280, 720 },
        //    .name = "WindowOutput",
        //};
        //vortex::OutputDesc ndi{
        //    .format = wis::DataFormat::RGBA8Unorm,
        //    .size = { 1280, 720 },
        //    .name = "Vortex",
        //};
        //_outputs.emplace_back(NodeFactory::CreateNode("NDIOutput", _gfx, reinterpret_cast<NodeDesc*>(&ndi)));
        //_outputs.emplace_back(NodeFactory::CreateNode("WindowOutput", _gfx, reinterpret_cast<NodeDesc*>(&window)));
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

    std::vector<std::unique_ptr<vortex::INode>> _outputs;

    // CEF client for UI
    vortex::ui::UIApp _ui_app;

    std::unique_ptr<vortex::INode> _image_input;

private:
    const AppExitControl& _exit;
};
} // namespace vortex