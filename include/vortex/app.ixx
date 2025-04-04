module;
#include <stdexcept>
#include <format>
#include <vector>
#include <memory>
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
        , _exit(AppExitControl::GetInstance())
    {
        _outputs.reserve(2);

        vortex::OutputDesc window{
            .format = wis::DataFormat::RGBA8Unorm,
            .size = { 1280, 720 },
            .name = "WindowOutput",
        };
        vortex::OutputDesc ndi{
            .format = wis::DataFormat::RGBA8Unorm,
            .size = { 1280, 720 },
            .name = "Vortex",
        };
        _outputs.emplace_back(NodeFactory::CreateNode("NDIOutput", _gfx, reinterpret_cast<NodeDesc*>(&ndi)));
        _outputs.emplace_back(NodeFactory::CreateNode("WindowOutput", _gfx, reinterpret_cast<NodeDesc*>(&window)));
    }

public:
    int Run()
    {
        while (!_exit.exit) {
        }
        return 0;
    }

private:
    vortex::NDILibrary _ndi;
    vortex::SDLLibrary _sdl;

    vortex::Graphics _gfx;

    std::vector<std::unique_ptr<vortex::INode>> _outputs;

private:
    const AppExitControl& _exit;
};
} // namespace vortex