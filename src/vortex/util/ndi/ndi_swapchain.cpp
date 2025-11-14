#include <vortex/util/ndi/ndi_swapchain.h>
#include <vortex/util/common.h>
#include <vortex/graphics.h>
#include <vortex/consts.h>
#include <numbers>

vortex::NDISwapchain::NDISwapchain(const vortex::Graphics& gfx, const NDISwapchainDesc& desc)
    : _conversion_resources(gfx)
    , _video_frame(int32_t(desc.width),
                   int32_t(desc.height),
                   GetNDIFormat(desc.format),
                   desc.framerate.num(),
                   desc.framerate.denom())
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
        vortex::error("Failed to create auxiliary command list for NDI swapchain: {}",
                      result.error);
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

void vortex::NDISwapchain::CopyToStagingBuffer(const vortex::Graphics& gfx,
                                               wis::CommandList& cmd_list,
                                               vortex::DescriptorBufferView dbv)
{
    // cmd is in state of rendering to the texture
    uint32_t index = _current_index;
    auto& resources = _conversion_resources.uget();
    auto& srv = _in_srvs[index];
    auto& current_texture = _textures[index];

    // Transition the texture to be readable
    // Close the render target
    cmd_list.TextureBarrier(
            {
                    .sync_before = wis::BarrierSync::RenderTarget,
                    .sync_after = wis::BarrierSync::Compute,
                    .access_before = wis::ResourceAccess::RenderTarget,
                    .access_after = wis::ResourceAccess::ShaderResource,
                    .state_before = wis::TextureState::RenderTarget,
                    .state_after = wis::TextureState::ShaderResource,
            },
            current_texture);

    // Bind the compute pipeline
    cmd_list.SetComputeRootSignature(resources._root_signature);
    cmd_list.SetPipelineState(resources._pipeline_state);

    // Set the DescBuffer
    dbv.WriteTexture(0, srv);
    dbv.WriteRWBuffer(1, _yuv_staging_buffer[index], 4, _video_frame.xres * _video_frame.yres / 2);
    dbv.BindComputeOffset(gfx, cmd_list, resources._root_signature, 0);

    // Execute the conversion
    // Each thread processes 2 pixels, thread group size is 16x16
    // So each thread group processes 32x16 pixels
    uint32_t width = static_cast<uint32_t>(_video_frame.xres);
    uint32_t height = static_cast<uint32_t>(_video_frame.yres);
    uint32_t groupsX = (width + 31) / 32;  // 32 pixels per group (16 threads * 2 pixels)
    uint32_t groupsY = (height + 15) / 16; // 16 pixels per group
    cmd_list.Dispatch(groupsX, groupsY, 1);

    // Transition buffer to copy source
    cmd_list.BufferBarrier(
            {
                    .sync_before = wis::BarrierSync::Compute,
                    .sync_after = wis::BarrierSync::Copy,
                    .access_before = wis::ResourceAccess::Common,
                    .access_after = wis::ResourceAccess::Common,
            },
            _yuv_staging_buffer[index]);

    // Copy to staging buffer
    wis::BufferRegion region = { .size_bytes = uint64_t(_video_frame.xres) *
                                         uint64_t(_video_frame.yres) * 2 };
    cmd_list.CopyBuffer(_yuv_staging_buffer[index], _staging_buffer[index], region);
}

auto vortex::NDISwapchain::CreateBuffers(const vortex::Graphics& gfx,
                                         uint32_t width,
                                         uint32_t height) noexcept -> wis::Result
{
    auto& allocator = gfx.GetAllocator();
    wis::Result result = wis::success;
    wis::TextureDesc desc{
        .format = _format,
        .size = { width, height },
        .layout = wis::TextureLayout::Texture2D,
        .usage = wis::TextureUsage::RenderTarget | wis::TextureUsage::ShaderResource |
                wis::TextureUsage::CopySrc,
    };
    wis::ShaderResourceDesc srv_desc{
        .format = _format,
        .view_type = wis::TextureViewType::Texture2D,
        .component_mapping = {},
        .subresource_range = { .base_mip_level = 0,
                              .level_count = 1,
                              .base_array_layer = 0,
                              .layer_count = 1 }
    };

    // Recreate the texture with the new size
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _textures[i] = allocator.CreateTexture(result, desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain texture: {}", result.error);
            return result;
        }

        _in_srvs[i] = gfx.GetDevice().CreateShaderResource(result, _textures[i], srv_desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain texture SRV: {}", result.error);
            return result;
        }

#ifdef VORTEX_DX12
        auto& tex_int = _textures[i].GetInternal().resource;
        tex_int->SetName(std::format(L"Vortex NDI Swapchain Texture {}", i).c_str());
#endif // VORTEX_DX12
    }

    // Initialize the staging buffers
    for (size_t i = 0; i < max_swapchain_images; ++i) {
        _yuv_staging_buffer[i] = allocator.CreateBuffer(
                result,
                uint64_t(width) * uint64_t(height) * 2, // UYVY is 2 bytes per pixel
                wis::BufferUsage::StorageBuffer | wis::BufferUsage::CopySrc,
                wis::MemoryType::DeviceLocal,
                wis::MemoryFlags::None);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain staging buffer: {}", result.error);
            return result;
        }

        _staging_buffer[i] = allocator.CreateBuffer(result,
                                                    uint64_t(width) * uint64_t(height) *
                                                            2, // UYVY is 2 bytes per pixel
                                                    wis::BufferUsage::CopyDst,
                                                    wis::MemoryType::Readback,
                                                    wis::MemoryFlags::Mapped);
        if (!vortex::success(result)) {
            vortex::error("Failed to create NDI swapchain readback buffer: {}", result.error);
            return result;
        }
    }

    // Map the first staging buffer
    // Assign in reverse order so that the first buffer is used for uploading
    _video_frame.p_data = _staging_buffer[1].Map<uint8_t>();
    _staging_data = _staging_buffer[0].Map<uint8_t>();

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
                    .state_after = wis::TextureState::ShaderResource,
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

//-----------------------------------------------------------------------------
vortex::NDIConversionLazy::NDIConversionLazy(const vortex::Graphics& gfx)
{
    wis::Result result = wis::success;
    auto& device = gfx.GetDevice();
    auto& desc_buf = gfx.GetDescriptorBufferExtension();

    // Create root signature
    wis::DescriptorTableEntry entries[] = {
        {  .type = wis::DescriptorType::Texture,
         .bind_register = 0,
         .binding = 0,
         .count = 1,
         .binding_space = 0 },
        { .type = wis::DescriptorType::RWBuffer,
         .bind_register = 0,
         .binding = 1,
         .count = 1,
         .binding_space = 0 }
    };
    wis::DescriptorTable table{
        .type = wis::DescriptorHeapType::Descriptor,
        .entries = entries,
        .entry_count = std::size(entries),
        .stage = wis::ShaderStages::All,
    };
    _root_signature = desc_buf.CreateRootSignature(result, nullptr, 0, nullptr, 0, &table, 1);
    if (!vortex::success(result)) {
        vortex::error("Failed to create NDI conversion root signature: {}", result.error);
        return;
    }

    auto shader = gfx.LoadShader("shaders/rgba_to_uyvy.cs");

    wis::ComputePipelineDesc pipeline_desc{ .root_signature = _root_signature, .shader = shader };
    _pipeline_state = device.CreateComputePipeline(result, pipeline_desc);
    if (!vortex::success(result)) {
        vortex::error("Failed to create NDI conversion pipeline state: {}", result.error);
        return;
    }
}
