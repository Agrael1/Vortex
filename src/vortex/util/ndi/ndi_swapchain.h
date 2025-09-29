#pragma once
#include <vortex/util/ndi/ndi_library.h>
#include <vortex/util/rational.h>
#include <wisdom/wisdom.hpp>

namespace vortex {
class Graphics;
struct NDISwapchainDesc {
    uint32_t width = 1280;
    uint32_t height = 720;
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm;
    vortex::ratio32_t framerate = { 60000, 1001 }; // 59.94 fps
    std::string_view name = "Vortex NDI Output";
};

class NDISwapchain
{
public:
    static inline constexpr uint32_t max_swapchain_images = 2;

public:
    NDISwapchain() = default;
    NDISwapchain(const vortex::Graphics& gfx, const NDISwapchainDesc& desc);
    ~NDISwapchain();

public:
    auto GetTextures() const noexcept -> std::span<const wis::Texture, max_swapchain_images>
    {
        return _textures;
    }
    void CopyToStagingBuffer(wis::CommandList& cmd_list)
    {
        // Copy the current texture to the staging buffer
        CopyToStagingBuffer(cmd_list, _current_index);
    }
    auto GetCurrentIndex() const noexcept -> uint32_t
    {
        return _current_index;
    }
    void SetFramerate(vortex::ratio32_t framerate) noexcept
    {
        _video_frame.frame_rate_N = framerate.num();
        _video_frame.frame_rate_D = framerate.denom();
    }
    void SetName(std::string_view name)
    {
        _send_instance.Recreate(name);
    }
    bool Resize(const vortex::Graphics& gfx, uint32_t width, uint32_t height);
    void SendAudio(std::span<const float> samples);
    void Present()
    {
        NDIlib_send_send_video_async_v2(_send_instance, &_video_frame);
        SwapBuffers();
    }

private:
    void CopyToStagingBuffer(wis::CommandList& cmd_list, uint32_t index)
    {
        wis::BufferTextureCopyRegion region{
            .texture = {
                    .size = { uint32_t(_video_frame.xres), uint32_t(_video_frame.yres), 1 },
                    .format = _format }
        };
        cmd_list.CopyTextureToBuffer(_textures[index], _staging_buffer[_current_index], &region, 1);
    }
    void SwapBuffers()
    {
        _current_index = (_current_index + 1) % max_swapchain_images;
        std::swap(_video_frame.p_data, _staging_data);
    }


    auto CreateBuffers(const vortex::Graphics& gfx, uint32_t width, uint32_t height) noexcept -> wis::Result;
    void DestroyBuffers() noexcept;

private:
    wis::Texture _textures[max_swapchain_images];
    wis::Buffer _staging_buffer[max_swapchain_images];
    wis::CommandList _command_list_aux;

    uint8_t* _staging_data = nullptr;
    uint32_t _current_index = 0;
    wis::DataFormat _format = wis::DataFormat::RGBA8Unorm;

    // NDI backend
    NDISendInstance _send_instance;
    NDIlib_video_frame_v2_t _video_frame = {};
};

} // namespace vortex