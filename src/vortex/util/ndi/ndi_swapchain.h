#pragma once
#include <vortex/util/ndi/ndi_library.h>
#include <wisdom/wisdom.hpp>


namespace vortex {
class Graphics;
class NDISwapchain
{
public:
    static inline constexpr uint32_t max_swapchain_images = 2;

public:
    NDISwapchain() = default;
    NDISwapchain(const vortex::Graphics& gfx, wis::Size2D size = { 1280, 720 }, wis::DataFormat format = wis::DataFormat::RGBA8Unorm, std::string_view name = "Vortex 1");
    ~NDISwapchain();

public:
    const wis::Texture& GetTexture() const noexcept
    {
        return _texture;
    }
    void Present(wis::CommandList& cmd_list)
    {
        if (!_send_instance) {
            vortex::error("NDI send instance is not initialized");
            return;
        }

        // Copy the current texture to the staging buffer
        CopyToStagingBuffer(cmd_list);
        // Send the frame asynchronously
        SendFrame();
    }
    uint32_t GetCurrentIndex() const noexcept
    {
        return _current_index;
    }
    bool Resize(const vortex::Graphics& gfx, uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) {
            vortex::error("NDI swapchain resize called with zero dimensions");
            return false;
        }

        // No need to resize if the dimensions are the same or lower than the current size
        if (width <= _video_frame.xres && height <= _video_frame.yres) {
            _video_frame.xres = int32_t(width);
            _video_frame.yres = int32_t(height);
            return false;
        }

        // Resize the texture and staging buffers (avoid fragmentation)
        _texture = {};
        CreateBuffers(gfx, width, height);
        return true;
    }
    void SetName(std::string_view name)
    {
        _send_instance.Recreate(name);
    }

private:
    void CopyToStagingBuffer(wis::CommandList& cmd_list)
    {
        wis::BufferTextureCopyRegion region{
            .texture = {
                    .size = { uint32_t(_video_frame.xres), uint32_t(_video_frame.yres), 1 },
                    .format = _format }
        };
        cmd_list.CopyTextureToBuffer(_texture, _staging_buffer[_current_index], &region, 1);
    }
    void SwapBuffers()
    {
        _current_index = (_current_index + 1) % max_swapchain_images;
        std::swap(_video_frame.p_data, _staging_data);
    }
    void SendFrame()
    {
        NDIlib_send_send_video_async_v2(_send_instance, &_video_frame);
        SwapBuffers();
    }

    void CreateBuffers(const vortex::Graphics& gfx, uint32_t width, uint32_t height);

private:
    wis::Texture _texture;
    wis::RenderTarget _render_targets[max_swapchain_images];
    wis::Buffer _staging_buffer[max_swapchain_images]; // more is ineffective
    uint8_t* _staging_data = nullptr;
    uint32_t _current_index = 0;
    wis::DataFormat _format = wis::DataFormat::RGBA8Unorm;

    // NDI backend
    NDISendInstance _send_instance;
    NDIlib_video_frame_v2_t _video_frame = {};
};
} // namespace vortex