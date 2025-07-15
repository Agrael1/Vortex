#pragma once
#include <vortex/node.h>
#include <vortex/probe.h>
#include <graaflib/graph.h>

namespace vortex {
class GraphModel
{
public:
    GraphModel() = default;

public:
    void CreateNode(const vortex::Graphics& gfx)
    {
        _nodes.emplace_back(std::make_unique<vortex::ImageInput>(
                gfx,
                vortex::ImageInput::ImageParams{
                        .data = {
                                .image_path = "ui/HDR.jpg",
                                .crop_rect = { 0.f, 0.f, 1.f, 1.f }, // Full image
                                .size = { 1280, 720 },
                                .origin = { 0, 0 },
                                .rotation = { 0.f, 0.f } } }));
    }
    void CreateOutput(const vortex::Graphics& gfx)
    {
        vortex::OutputDesc out_desc{
            .input = vortex::NodeInput::OUTPUT_DESC,
            .format = wis::DataFormat::RGBA8Unorm, // Most common format for desktop applications
            .size = { 1280, 720 }, // Default size, can be changed later
            .name = "Main Output"
        };
        _outputs.emplace_back(std::make_unique<vortex::WindowOutput>(gfx, out_desc));
    }
    void TraverseNodes(vortex::RenderProbe& probe)
    {
        probe._output_size = { 1280, 720 }; // Set output size for the probe

        auto& cmd_list = probe._command_list;
        auto& queue = probe._gfx.GetMainQueue();
        cmd_list.Reset();

        auto& output = static_cast<WindowOutput&>(*_outputs[0].get());

        uint32_t index = output._swapchain.GetCurrentIndex();
        auto& texture = output._textures[index];

        cmd_list.TextureBarrier(
            wis::TextureBarrier{
                        .sync_before = wis::BarrierSync::None,
                        .sync_after = wis::BarrierSync::RenderTarget,
                        .access_before = wis::ResourceAccess::NoAccess,
                        .access_after = wis::ResourceAccess::RenderTarget,
                        .state_before = wis::TextureState::Present,
                        .state_after = wis::TextureState::RenderTarget,
            },
                texture);

        wis::RenderPassRenderTargetDesc rtd{
            .target = output._render_targets[index],
            .load_op = wis::LoadOperation::Clear,
            .store_op = wis::StoreOperation::Store,
            .clear_value = { 0.f, 0.f, 0.f, 1.f } // Clear to black
        };
        wis::RenderPassDesc rpd{
            .target_count = 1,
            .targets = &rtd,
        };
        cmd_list.BeginRenderPass(rpd);
        probe._descriptor_buffer.BindBuffers(probe._gfx, cmd_list); // Bind descriptor buffers

        _nodes[0]->Visit(probe); // Visit the first node (ImageInput)

        cmd_list.EndRenderPass();
        cmd_list.TextureBarrier(
            wis::TextureBarrier{
                        .sync_before = wis::BarrierSync::RenderTarget,
                        .sync_after = wis::BarrierSync::None,
                        .access_before = wis::ResourceAccess::RenderTarget,
                        .access_after = wis::ResourceAccess::NoAccess,
                        .state_before = wis::TextureState::RenderTarget,
                        .state_after = wis::TextureState::Present,
            },
                texture);

        cmd_list.Close();

        wis::CommandListView command_lists[] = { cmd_list };
        queue.ExecuteCommandLists(command_lists, 1);

        output._swapchain.Present();
    }
    void ConnectNodes()
    {
        _connections.push_back(vortex::Connection{
                _nodes[0].get(),
                _outputs[0].get() });

        _graph.add_edge(
            'a',
            'b',
                std::move(_connections.back()));
    }

private:
    std::vector<std::unique_ptr<vortex::INode>> _nodes;
    std::vector<std::unique_ptr<vortex::INode>> _outputs;
    std::vector<vortex::Connection> _connections;

    graaf::directed_graph<vortex::INode*, vortex::Connection> _graph; ///< Graph structure to hold nodes and connections
    uint32_t frame = 0; ///< Frame counter for rendering
};
} // namespace vortex