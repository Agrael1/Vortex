#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/ndi/ndi_swapchain.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>


namespace vortex {
class NDIOutput : public vortex::graph::OutputImpl<NDIOutput, NDIOutputProperties>
{
    static constexpr wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for render targets
public:
    NDIOutput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props), _swapchain(gfx, { window_size.x, window_size.y }, format, name)
    {
        wis::Result result = wis::success;
        // Create the render target for the NDI output
        wis::RenderTargetDesc rt_desc{
            .format = format,
        };
        _render_target = gfx.GetDevice().CreateRenderTarget(result, _swapchain.GetTexture(), rt_desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create render target for NDIOutput: {}", result.error);
            return;
        }
        // Initialize command lists for each swapchain image
        for (size_t i = 0; i < NDISwapchain::max_swapchain_images; ++i) {
            _command_lists[i] = gfx.GetDevice().CreateCommandList(result, wis::QueueType::Graphics);
            if (!vortex::success(result)) {
                vortex::error("Failed to create command list for NDIOutput: {}", result.error);
                return;
            }
        }

        auto& cmd_list = _command_lists[0];
        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::None,
                                        .sync_after = wis::BarrierSync::None,
                                        .access_before = wis::ResourceAccess::NoAccess,
                                        .access_after = wis::ResourceAccess::NoAccess,
                                        .state_before = wis::TextureState::Undefined,
                                        .state_after = wis::TextureState::CopySource,
                                },
                                _swapchain.GetTexture());
        cmd_list.Close();
        wis::CommandListView views[]{ cmd_list };
        gfx.GetMainQueue().ExecuteCommandLists(views, 1);
        gfx.WaitForGPU(); // Ensure the command list is executed before proceeding
    }

public:
    // Properties
    void SetName(std::string_view value, bool notify = false)
    {
        NDIOutputProperties::SetName(value, notify);
        if (IsInitialized()) {
            _swapchain.SetName(value);
        }
    }
    void SetWindowSize(DirectX::XMUINT2 value, bool notify = false)
    {
        NDIOutputProperties::SetWindowSize(value, notify);
        _resized = IsInitialized();
    }

public:
    virtual bool IsReady() const noexcept override
    {
        // Check if the swapchain is ready for rendering
        return true;
    }
    virtual vortex::ratio32_t GetOutputFPS() const noexcept
    {
        return GetFramerate();
    }
    virtual wis::Size2D GetOutputSize() const noexcept
    {
        return { window_size.x, window_size.y };
    }

    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override
    {
        if (!IsReady()) {
            vortex::error("NDIOutput is not ready for rendering");
            return;
        }

        if (_resized) {
            // Resize the swapchain and render target
            if (_swapchain.Resize(gfx, window_size.x, window_size.y)) {
                _render_target = gfx.GetDevice().CreateRenderTarget(_swapchain.GetTexture(), {}).value;
            }
            _resized = false;
        }

        auto current_texture_index = _swapchain.GetCurrentIndex();
        probe._command_list = &_command_lists[current_texture_index];

        // Pass to the sink nodes for post-order processing
        RenderPassForwardDesc desc{
            ._current_rt_view = _render_target,
            ._current_rt_texture = &_swapchain.GetTexture(),
            ._output_size = { window_size.x, window_size.y }
        };

        // Barrier to ensure the render target is ready for rendering
        auto& cmd_list = *probe._command_list;
        std::ignore = cmd_list.Reset();
        probe._descriptor_buffer.BindBuffers(gfx, cmd_list);

        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::Copy,
                                        .sync_after = wis::BarrierSync::RenderTarget,
                                        .access_before = wis::ResourceAccess::CopySource,
                                        .access_after = wis::ResourceAccess::RenderTarget,
                                        .state_before = wis::TextureState::CopySource,
                                        .state_after = wis::TextureState::RenderTarget,
                                },
                                _swapchain.GetTexture());

        // Pass to the next nodes in the graph
        for (auto& sink : _sinks.GetSinks() | std::views::filter([](auto& sink) { return sink; })) {
            sink.source_node->Evaluate(gfx, probe, &desc);
        }

        // Close the render target
        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::RenderTarget,
                                        .sync_after = wis::BarrierSync::Copy,
                                        .access_before = wis::ResourceAccess::RenderTarget,
                                        .access_after = wis::ResourceAccess::CopySource,
                                        .state_before = wis::TextureState::RenderTarget,
                                        .state_after = wis::TextureState::CopySource,
                                },
                                _swapchain.GetTexture());

        _swapchain.Present(_command_lists[current_texture_index]);

        // End the command list
        if (!cmd_list.Close()) {
            vortex::error("Failed to close command list for WindowOutput");
            return;
        }

        wis::CommandListView views[]{ cmd_list };
        gfx.GetMainQueue().ExecuteCommandLists(views, 1);
    }

private:
    NDISwapchain _swapchain;
    wis::CommandList _command_lists[NDISwapchain::max_swapchain_images];
    wis::RenderTarget _render_target;

    bool _resized = false; ///< Flag to indicate if the output has been resized
};
} // namespace vortex