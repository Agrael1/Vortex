#include <vortex/util/ndi/ndi_swapchain.h>
#include <vortex/util/common.h>
#include <vortex/graphics.h>
#include <vortex/consts.h>
#include <numbers>

vortex::NDISwapchain::NDISwapchain(const vortex::Graphics& gfx, const NDISwapchainDesc& desc)
    : _video_frame(
              int32_t(desc.width),
              int32_t(desc.height),
              GetNDIFormat(desc.format),
              desc.framerate.num(), desc.framerate.denom())
    , _format(desc.format)
    , _send_instance(desc.name)
{
    if (!_send_instance) {
        vortex::error("Failed to create NDI send instance");
        return;
    }

    wis::Result result = wis::success;
    _command_list_aux = gfx.GetDevice().CreateCommandList(result, wis::QueueType::Graphics);
    if (!vortex::success(result)) {
        vortex::error("Failed to create auxiliary command list for NDI swapchain: {}", result.error);
        return;
    }

    CreateBuffers(gfx, desc.width, desc.height);
}
vortex::NDISwapchain::~NDISwapchain()
{
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _staging_buffer[i].Unmap();
    }
}

auto vortex::NDISwapchain::CreateBuffers(const vortex::Graphics& gfx, uint32_t width, uint32_t height) noexcept -> wis::Result
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
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _textures[i] = allocator.CreateTexture(result, desc);

        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain texture: {}", result.error);
            return result;
        }

#ifdef VORTEX_DX12
        auto& tex_int = _textures[i].GetInternal().resource;
        tex_int->SetName(std::format(L"Vortex NDI Swapchain Texture {}", i).c_str());
#endif // VORTEX_DX12
    }

    // Initialize the staging buffers
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _staging_buffer[i] = allocator.CreateReadbackBuffer(result, alloc_info.size_bytes);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain staging buffer: {}", result.error);
            return result;
        }
    }

    // Map the first staging buffer
    _video_frame.p_data = _staging_buffer[0].Map<uint8_t>();
    _staging_data = _staging_buffer[1].Map<uint8_t>();

    // Transition all textures to copy source layout
    wis::TextureBarrier2 barriers[NDISwapchain::max_swapchain_images];
    for (size_t i = 0; i < NDISwapchain::max_swapchain_images; ++i) {
        barriers[i] = {
            .barrier = {
                    .sync_before = wis::BarrierSync::None,
                    .sync_after = wis::BarrierSync::None,
                    .access_before = wis::ResourceAccess::NoAccess,
                    .access_after = wis::ResourceAccess::NoAccess,
                    .state_before = wis::TextureState::Undefined,
                    .state_after = wis::TextureState::CopySource,
            },
            .texture = _textures[i],
        };
    }
    std::ignore = _command_list_aux.Reset();
    _command_list_aux.TextureBarriers(barriers, NDISwapchain::max_swapchain_images);
    _command_list_aux.Close();

    gfx.ExecuteCommandLists({ _command_list_aux });
    gfx.WaitForGPU();
    return wis::success;
}
bool vortex::NDISwapchain::Resize(const vortex::Graphics& gfx, uint32_t width, uint32_t height)
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

    DestroyBuffers();
    CreateBuffers(gfx, width, height);
    return true;
}

static int it = 10;
void vortex::NDISwapchain::SendAudio(std::span<const float> samples)
{
    if (!_send_instance) {
        return;
    }

    NDIlib_audio_frame_v3_t audio_frame = {};
    audio_frame.sample_rate = 48000;
    audio_frame.no_channels = 2;
    audio_frame.no_samples = samples.size() / 2;
    audio_frame.timecode = NDIlib_send_timecode_synthesize;
    audio_frame.FourCC = NDIlib_FourCC_audio_type_FLTP;
    audio_frame.p_data = (uint8_t*)samples.data();
    audio_frame.channel_stride_in_bytes = audio_frame.no_samples * sizeof(float);
    NDIlib_send_send_audio_v3(_send_instance, &audio_frame);
}
void vortex::NDISwapchain::DestroyBuffers() noexcept
{
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _staging_buffer[i].Unmap();
        _staging_buffer[i] = {};
        _textures[i] = {};
    }
    _staging_data = nullptr;
    _video_frame.p_data = nullptr;
}