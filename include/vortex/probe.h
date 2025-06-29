#pragma once

namespace vortex {
class Graphics;
class PipelineStorage;

class RenderProbe
{
public:
    const Graphics& _gfx;
    PipelineStorage& _pipeline_storage;

    wis::CommandList& _command_list;

    wis::DescriptorStorageView _storage_view;
    wis::RenderTargetView _current_rt_view;
    const wis::Texture& _current_rt_texture;

    wis::Size2D _output_size; // Useful for different render targets
};
} // namespace vortex