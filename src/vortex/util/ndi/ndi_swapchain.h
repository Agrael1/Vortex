#pragma once
#include <vortex/util/ndi/ndi_library.h>
#include <vortex/util/rational.h>
#include <vortex/util/lazy.h>
#include <vortex/gfx/descriptor_buffer.h>
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

class NDIConversionLazy
{
public:
    NDIConversionLazy(const vortex::Graphics& gfx);

public:
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Compute pipeline state for conversion
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
    void CopyToStagingBuffer(const vortex::Graphics& gfx,
                             wis::CommandList& cmd_list,
                             vortex::DescriptorBufferView dbv);
    auto GetCurrentIndex() const noexcept -> uint32_t { return _current_index; }
    void SetFramerate(vortex::ratio32_t framerate) noexcept
    {
        _video_frame.frame_rate_N = framerate.num();
        _video_frame.frame_rate_D = framerate.denom();
    }
    void SetName(std::string_view name) { _send_instance.Recreate(name); }
    bool Resize(const vortex::Graphics& gfx, uint32_t width, uint32_t height);
    void SendAudio(std::span<const float> samples);
    void Present()
    {
        NDIlib_send_send_video_async_v2(_send_instance, &_video_frame);
        SwapBuffers();
    }

private:
    void SwapBuffers()
    {
        _current_index = (_current_index + 1) % max_swapchain_images;
        std::swap(_video_frame.p_data, _staging_data);
    }

    auto CreateBuffers(const vortex::Graphics& gfx, uint32_t width, uint32_t height) noexcept
            -> wis::Result;
    void DestroyBuffers() noexcept;

    void DX12SetComputeDescriptorTableOffset(
            wis::DX12CommandListView cmd_list,
            wis::DX12RootSignatureView root_signature,
            uint32_t root_table_index,
            wis::DX12DescriptorBufferGPUView buffer,
                                             uint32_t table_aligned_byte_offset) const noexcept;

private:
    [[no_unique_address]] vortex::lazy_ptr<NDIConversionLazy> _conversion_resources;

    wis::Texture _textures[max_swapchain_images]; // RGBA8 textures for rendering
    wis::ShaderResource _in_srvs[max_swapchain_images]; // SRVs for RGBA textures
    wis::Buffer _yuv_staging_buffer[max_swapchain_images]; // Buffer for YUV conversion
    wis::Buffer _staging_buffer[max_swapchain_images]; // Readback buffers
    wis::CommandList _command_list_aux;

    uint8_t* _staging_data = nullptr;
    uint32_t _current_index = 0;
    wis::DataFormat _format = wis::DataFormat::RGBA8Unorm;

    // NDI backend
    NDISendInstance _send_instance;
    NDIlib_video_frame_v2_t _video_frame = {};
};

} // namespace vortex