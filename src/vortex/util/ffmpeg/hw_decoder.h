#pragma once
#include <wisdom/wisdom.hpp>
#include <vortex/util/ffmpeg/error.h>
#include <vortex/util/ffmpeg/types.h>
#include <expected>

extern "C" {
#ifdef WISDOM_DX12
#include <libavutil/hwcontext_d3d12va.h>
#endif
#ifdef WISDOM_VULKAN
#include <libavutil/hwcontext_vulkan.h>
#endif
}

#ifdef WISDOM_DX12
namespace vortex::ffmpeg {
class DX12VADecodeContext;
} // namespace vortex::ffmpeg

namespace wis {
template<>
struct Internal<vortex::ffmpeg::DX12VADecodeContext> {
    wis::com_ptr<ID3D12Device> d3d12_device;
    vortex::ffmpeg::unique_buffer hw_device_ctx;
};
} // namespace wis

namespace vortex::ffmpeg {
class DX12VADecodeContext : public wis::QueryInternal<DX12VADecodeContext>
{
public:
    DX12VADecodeContext() = default;

public:
    [[nodiscard]] AVBufferRef* GetHWDeviceContext() const noexcept
    {
        return GetInternal().hw_device_ctx.get();
    }
    std::expected<vortex::ffmpeg::unique_buffer, std::error_code>
    CreateHWFramesContext(int width, int height, AVPixelFormat format) const noexcept;
};

inline std::expected<DX12VADecodeContext, std::error_code>
DX12CreateDecodeContext(const wis::DX12Device& device) noexcept
{
    DX12VADecodeContext ctx;
    auto& internal = ctx.GetMutableInternal();
    internal.hw_device_ctx.reset(av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D12VA));
    if (!internal.hw_device_ctx) {
        return std::unexpected(make_ffmpeg_error(AVERROR(ENOMEM)));
    }
    internal.d3d12_device = device.GetInternal().device;

    AVHWDeviceContext* hw_device_ctx_data = std::launder(reinterpret_cast<AVHWDeviceContext*>(internal.hw_device_ctx->data));
    AVD3D12VADeviceContext* d3d12va_ctx = std::launder(reinterpret_cast<AVD3D12VADeviceContext*>(hw_device_ctx_data->hwctx));
    d3d12va_ctx->device = internal.d3d12_device.get();
    internal.d3d12_device->AddRef(); // FFmpeg will release it

    if (int ret = av_hwdevice_ctx_init(internal.hw_device_ctx.get()); ret < 0) {
        return std::unexpected(make_ffmpeg_error(ret));
    }
    return std::move(ctx);
}
inline std::expected<vortex::ffmpeg::unique_buffer, std::error_code>
DX12VADecodeContext::CreateHWFramesContext(int width, int height, AVPixelFormat format) const noexcept
{
    vortex::ffmpeg::unique_buffer hw_frames_ctx{ av_hwframe_ctx_alloc(GetHWDeviceContext()) };
    if (!hw_frames_ctx) {
        return std::unexpected(make_ffmpeg_error(AVERROR(ENOMEM)));
    }
    AVHWFramesContext* frames_ctx_data = std::launder(reinterpret_cast<AVHWFramesContext*>(hw_frames_ctx->data));
    AVD3D12VAFramesContext* d3d12va_frames_ctx = std::launder(reinterpret_cast<AVD3D12VAFramesContext*>(frames_ctx_data->hwctx));

    frames_ctx_data->format = AV_PIX_FMT_D3D12;
    frames_ctx_data->sw_format = format;
    frames_ctx_data->width = width;
    frames_ctx_data->height = height;
    frames_ctx_data->initial_pool_size = 32; // Increased pool size
    d3d12va_frames_ctx->flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE; // No special flags

    if (int ret = av_hwframe_ctx_init(hw_frames_ctx.get()); ret < 0) {
        return std::unexpected(make_ffmpeg_error(ret));
    }

    return std::move(hw_frames_ctx);
}

inline std::expected<wis::DX12Texture, std::error_code>
GetTextureFromFrame(const AVFrame& frame) noexcept
{
    if (frame.format != AV_PIX_FMT_D3D12) {
        return std::unexpected(make_ffmpeg_error(AVERROR(EINVAL)));
    }
    auto* d3d12_surface = reinterpret_cast<ID3D12Resource*>(frame.data[0]);
    if (!d3d12_surface) {
        return std::unexpected(make_ffmpeg_error(AVERROR(EINVAL)));
    }
    wis::DX12Texture texture;
    auto& internal = texture.GetMutableInternal();
    internal.resource = wis::com_ptr(d3d12_surface);
    return std::move(texture);
}
} // namespace vortex::ffmpeg
#endif

#ifdef WISDOM_VULKAN
namespace vortex::ffmpeg {
class VKVADecodeContext;
} // namespace vortex::ffmpeg
namespace wis {
template<>
struct Internal<vortex::ffmpeg::VKVADecodeContext> {
    wis::SharedDevice vulkan_device;
    vortex::ffmpeg::unique_buffer hw_device_ctx;
};
} // namespace wis
namespace vortex::ffmpeg {
class VKVADecodeContext : public wis::QueryInternal<VKVADecodeContext>
{
public:
    VKVADecodeContext() = default;

public:
    [[nodiscard]] AVBufferRef* GetHWDeviceContext() const noexcept
    {
        return GetInternal().hw_device_ctx.get();
    }
    std::expected<vortex::ffmpeg::unique_buffer, std::error_code>
    CreateHWFramesContext(int width, int height, AVPixelFormat format)const noexcept;
};
inline std::expected<VKVADecodeContext, std::error_code>
VKCreateDecodeContext(const wis::VKDevice& device) noexcept
{
    VKVADecodeContext ctx;
    auto& internal = ctx.GetMutableInternal();
    auto& device_internal = device.GetInternal();
    internal.hw_device_ctx.reset(av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VULKAN));
    if (!internal.hw_device_ctx) {
        return std::unexpected(make_ffmpeg_error(AVERROR(ENOMEM)));
    }

    internal.vulkan_device = device_internal.device;
    AVHWDeviceContext* hw_device_ctx_data = std::launder(reinterpret_cast<AVHWDeviceContext*>(internal.hw_device_ctx->data));
    AVVulkanDeviceContext* vulkan_ctx = std::launder(reinterpret_cast<AVVulkanDeviceContext*>(hw_device_ctx_data->hwctx));

    auto& adapter = device_internal.adapter.GetInternal();
    auto& instance = adapter.instance;
    vulkan_ctx->get_proc_addr = instance.gtable().vkGetInstanceProcAddr;
    vulkan_ctx->inst = instance.get();
    vulkan_ctx->phys_dev = adapter.adapter;
    vulkan_ctx->act_dev = internal.vulkan_device.get();

    instance.table().vkGetPhysicalDeviceFeatures2(vulkan_ctx->phys_dev, &vulkan_ctx->device_features);
    vulkan_ctx->enabled_inst_extensions = nullptr;
    vulkan_ctx->nb_enabled_inst_extensions = 0;

    // TODO: Enable required device extensions, make extension for this
    vulkan_ctx->enabled_dev_extensions = nullptr;
    vulkan_ctx->nb_enabled_dev_extensions = 0;

    int i = 0;
    for (const auto& q : device_internal.queues.available_queues) {
        auto& qf = vulkan_ctx->qf[i];

        if (q.queue_flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
            qf.idx = q.family_index;
            qf.num = q.count;
            qf.flags = static_cast<VkQueueFlagBits>(q.queue_flags);
            qf.video_caps = VK_VIDEO_CODEC_OPERATION_FLAG_BITS_MAX_ENUM_KHR;
        } else {
            qf.idx = q.family_index;
            qf.num = q.count;
            qf.flags = static_cast<VkQueueFlagBits>(q.queue_flags);
            qf.video_caps = VK_VIDEO_CODEC_OPERATION_NONE_KHR;
        }
        i++;
    }
    vulkan_ctx->nb_qf = i;

    if (int ret = av_hwdevice_ctx_init(internal.hw_device_ctx.get()); ret < 0) {
        return std::unexpected(make_ffmpeg_error(ret));
    }
    return std::move(ctx);
}

inline std::expected<vortex::ffmpeg::unique_buffer, std::error_code> 
VKVADecodeContext::CreateHWFramesContext(int width, int height, AVPixelFormat format) const noexcept
{
    vortex::ffmpeg::unique_buffer hw_frames_ctx{ av_hwframe_ctx_alloc(GetHWDeviceContext()) };
    if (!hw_frames_ctx) {
        return std::unexpected(make_ffmpeg_error(AVERROR(ENOMEM)));
    }
    AVHWFramesContext* frames_ctx_data = std::launder(reinterpret_cast<AVHWFramesContext*>(hw_frames_ctx->data));
    AVVulkanFramesContext* vulkan_frames_ctx = std::launder(reinterpret_cast<AVVulkanFramesContext*>(frames_ctx_data->hwctx));
    frames_ctx_data->format = AV_PIX_FMT_VULKAN;
    frames_ctx_data->sw_format = format;
    frames_ctx_data->width = width;
    frames_ctx_data->height = height;
    frames_ctx_data->initial_pool_size = 32; // Increased pool size
    vulkan_frames_ctx->flags = AV_VK_FRAME_FLAG_NONE; // No special flags
    if (int ret = av_hwframe_ctx_init(hw_frames_ctx.get()); ret < 0) {
        return std::unexpected(make_ffmpeg_error(ret));
    }
    return std::move(hw_frames_ctx);
}
} // namespace vortex::ffmpeg
#endif

namespace vortex::ffmpeg {
#if defined(WISDOM_DX12) && !defined(WISDOM_FORCE_VULKAN)
using VADecodeContext = DX12VADecodeContext;
inline std::expected<VADecodeContext, std::error_code>
CreateDecodeContext(const wis::DX12Device& device) noexcept
{
    return DX12CreateDecodeContext(device);
}
#elif defined(WISDOM_VULKAN)
using VADecodeContext = VKVADecodeContext;
inline std::expected<VADecodeContext, std::error_code>
CreateDecodeContext(const wis::VKDevice& device) noexcept
{
    return VKCreateDecodeContext(device);
}
#endif // WISDOM_DX12

} // namespace vortex::ffmpeg