#pragma once
#ifdef NDI_AVAILABLE
#include <Processing.NDI.Lib.h>
#endif // NDI_AVAILABLE

#include <span>
#include <stdexcept>
#include <vortex/util/common.h>
#include <vortex/util/log.h>
#include <vortex/graph/interfaces.h>

namespace vortex {
class NDILibrary
{
public:
    NDILibrary()
    {
        int result = 0;
#ifdef NDI_AVAILABLE
        result = NDIlib_initialize();
#endif
        if (result == 0) {
            vortex::error("NDI library initialization failed");
        } else {
            vortex::info("NDI library initialized successfully");
        }
    }
    ~NDILibrary()
    {
#ifdef NDI_AVAILABLE
        NDIlib_destroy();
#endif
    }
};

#ifdef NDI_AVAILABLE

constexpr inline NDIlib_FourCC_video_type_e GetNDIFormat(wis::DataFormat format) noexcept
{
    switch (format) {
    case wis::DataFormat::RGBA8Unorm:
        return NDIlib_FourCC_video_type_RGBA;
    case wis::DataFormat::BGRA8Unorm:
        return NDIlib_FourCC_video_type_BGRA;
    default:
        return NDIlib_FourCC_video_type_RGBA;
    }
}

class NDISwapchain
{
    static inline constexpr uint32_t max_swapchain_images = 2;

public:
    NDISwapchain() = default;
    NDISwapchain(const vortex::Graphics& gfx, wis::Size2D size = { 1280, 720 }, wis::DataFormat format = wis::DataFormat::RGBA8Unorm, std::string_view name = "Vortex 1")
        : _video_frame(
                  int32_t(size.width),
                  int32_t(size.height),
                  GetNDIFormat(format))
        , _format(format)

    {
        auto& allocator = gfx.GetAllocator();
        wis::Result result = wis::success;
        wis::TextureDesc desc{
            .format = format,
            .size = { size.width, size.height },
            .layout = wis::TextureLayout::Texture2D,
            .usage = wis::TextureUsage::RenderTarget | wis::TextureUsage::ShaderResource | wis::TextureUsage::CopySrc,
        };
        wis::AllocationInfo alloc_info = allocator.GetTextureAllocationInfo(desc);

        _texture = allocator.CreateTexture(result, desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain texture: {}", result.error);
        }

        // Initialize the staging buffers
        for (size_t i = 0; i < max_swapchain_images; ++i) {
            _staging_buffer[i] = allocator.CreateReadbackBuffer(result, alloc_info.size_bytes);
            if (!vortex::success(result)) {
                vortex::error("Failed to create NDI swapchain staging buffer: {}", result.error);
            }
        }

        // Map the first staging buffer
        _video_frame.p_data = _staging_buffer[0].Map<uint8_t>();
        _staging_data = _staging_buffer[1].Map<uint8_t>();

        // Create the NDI send instance
        NDIlib_send_create_t create_desc = {};
        create_desc.p_ndi_name = name.data();
        _send_instance = NDIlib_send_create(&create_desc);
    }

    ~NDISwapchain()
    {
        for (size_t i = 0; i < max_swapchain_images; ++i) {
            _staging_buffer[i].Unmap();
        }
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

private:
    wis::Texture _texture;
    wis::Buffer _staging_buffer[max_swapchain_images]; // more is ineffective
    uint8_t* _staging_data = nullptr;
    uint32_t _current_index = 0;
    wis::DataFormat _format = wis::DataFormat::RGBA8Unorm;

    // NDI backend
    NDIlib_send_instance_t _send_instance = nullptr;
    NDIlib_video_frame_v2_t _video_frame = {};
};
#endif // NDI_AVAILABLE

struct NDIOutputProperties
{
    wis::Size2D size = { 1280, 720 };
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm;
    std::string_view name = "Vortex NDI Output";
};

class NDIOutput : public vortex::graph::OutputImpl<NDIOutput, NDIOutputProperties>
{
public:
    NDIOutput(const vortex::Graphics& gfx)
#ifdef NDI_AVAILABLE
        : _swapchain(gfx)
#endif

    {
    }

private:
#ifdef NDI_AVAILABLE
    NDISwapchain _swapchain;
#endif
};
} // namespace vortex