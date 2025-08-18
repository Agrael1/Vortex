#pragma once
#include <unordered_map>

namespace vortex {
class Graphics;
class DescriptorBuffer;

class RenderProbe
{
public:
    vortex::DescriptorBuffer& _descriptor_buffer;
    wis::CommandList* _command_list = nullptr; // Command list for recording commands

    wis::RenderTargetView _current_rt_view;
    const wis::Texture* _current_rt_texture;
    wis::Size2D _output_size; // Useful for different render targets
    uint32_t frame = 0;
};

struct RenderPassForwardDesc {
    wis::RenderTargetView _current_rt_view;
    const wis::Texture* _current_rt_texture; // Must be in RT mode
    wis::Size2D _output_size; // Viewport
};
} // namespace vortex