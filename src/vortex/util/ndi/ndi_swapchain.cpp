#include <vortex/util/ndi/ndi_swapchain.h>
#include <vortex/util/common.h>
#include <vortex/graphics.h>

vortex::NDISwapchain::NDISwapchain(const vortex::Graphics& gfx, wis::Size2D size, wis::DataFormat format, std::string_view name)
    : _video_frame(
              int32_t(size.width),
              int32_t(size.height),
              GetNDIFormat(format))
    , _format(format)
    , _send_instance(name)
{
    CreateBuffers(gfx, size.width, size.height);
}

vortex::NDISwapchain::~NDISwapchain()
{
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _staging_buffer[i].Unmap();
    }
}

void vortex::NDISwapchain::CreateBuffers(const vortex::Graphics& gfx, uint32_t width, uint32_t height)
{
    auto& allocator = gfx.GetAllocator();
    wis::Result result = wis::success;
    wis::TextureDesc desc{
        .format = _format,
        .size = { width, height },
        .layout = wis::TextureLayout::Texture2D,
        .usage = wis::TextureUsage::RenderTarget | wis::TextureUsage::ShaderResource | wis::TextureUsage::CopySrc,
    };
    wis::AllocationInfo alloc_info = allocator.GetTextureAllocationInfo(desc);

    // Recreate the texture with the new size
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
}
