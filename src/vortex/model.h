#pragma once
#include <vortex/node.h>
#include <vortex/probe.h>
#include <graaflib/graph.h>
#include <atomic>
#include <unordered_set>

#include <graaflib/algorithm/graph_traversal/depth_first_search.h>

namespace vortex {
class GraphModel
{
public:
    GraphModel() = default;

public:
    uintptr_t CreateNode(const vortex::Graphics& gfx, std::string_view node_name)
    {
        auto node = NodeFactory::CreateNode(node_name, gfx);
        if (node) {
            vortex::error("Failed to create node: {}", node_name);
            return 0; // Return 0 if node creation failed
        }
        if (node->GetType() == vortex::NodeType::Output) {
            _outputs.push_back(node.get()); // Add to outputs if it's an output node
        }

        auto node_ptr = std::bit_cast<uintptr_t>(node.get());
        _graph.add_vertex(std::move(node), node_ptr);
        return node_ptr; // Return the pointer to the created node
    }

    void RemoveNode(uintptr_t node_ptr, bool immediate = false)
    {
        if (!_graph.has_vertex(node_ptr)) {
            vortex::error("Node not found in the graph: {}", node_ptr);
            return; // Node not found, nothing to remove
        }
        vortex::INode* node = std::bit_cast<vortex::INode*>(node_ptr);

        // Remove from dirty set when deleting
        _dirty_nodes.erase(node_ptr);
        if (node->GetType() == vortex::NodeType::Output) {
            if (auto output_it = std::ranges::find(_outputs, node); output_it != _outputs.end()) {
                _outputs.erase(output_it); // Remove from outputs if it's an output node
            } else {
                vortex::error("Output node not found in outputs: {}", node_ptr);
            }
        }
        _graph.remove_vertex(node_ptr); // Remove the node from the graph
    }

    void SetNodeProperty(uintptr_t node_ptr, uint32_t index, std::string_view value, bool notify_ui = false)
    {
        if (!_graph.has_vertex(node_ptr)) {
            vortex::error("Node not found in the graph: {}", node_ptr);
            return; // Node not found, nothing to remove
        }
        vortex::INode* node = std::bit_cast<vortex::INode*>(node_ptr);

        // Mark node as dirty and queue update message only if not already dirty
        _dirty_nodes.insert(node_ptr).second;
        node->SetProperty(index, value, notify_ui);
    }

    void TraverseNodes(vortex::RenderProbe& probe)
    {
        // Process all pending updates before rendering
        ProcessUpdates();

        probe._output_size = { 1280, 720 }; // Set output size for the probe

        auto& cmd_list = probe._command_list;
        auto& queue = probe._gfx.GetMainQueue();
        cmd_list.Reset();

        auto& output = static_cast<WindowOutput&>(*_outputs[0]);

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

        //_nodes[0]->Visit(probe); // Visit the first node (ImageInput)

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

    void ConnectNodes(uintptr_t node_ptr_left, uintptr_t node_ptr_right)
    {
        if (!_graph.has_vertex(node_ptr_left) || !_graph.has_vertex(node_ptr_right)) {
            vortex::error("One or both nodes not found in the graph: {} -> {}", node_ptr_left, node_ptr_right);
            return; // One or both nodes not found, nothing to connect
        }
        vortex::INode* left_node = std::bit_cast<vortex::INode*>(node_ptr_left);
        vortex::INode* right_node = std::bit_cast<vortex::INode*>(node_ptr_right);
        // Create a connection and add it to the graph
        vortex::Connection connection{ left_node, right_node };
        _graph.add_edge(node_ptr_left, node_ptr_right, connection);
    }

private:
    void ProcessUpdates()
    {
        // QueueMessage message;
        // while (_update_queue.pop(message)) {
        //     switch (message.type) {
        //     case UpdateType::Property: {
        //         uintptr_t node_ptr = std::bit_cast<uintptr_t>(message.GetNode());
        //         // Remove from dirty set when processing
        //         _dirty_nodes.erase(node_ptr);
        //
        //         // Commit staged properties to render properties
        //         auto it = _nodes.find(node_ptr);
        //         if (it != _nodes.end()) {
        //             auto& node = *it->second;
        //             node.CommitProperties(); // This would be implemented in the node
        //         }
        //         break;
        //     }
        //     case UpdateType::Create:
        //         // Handle node creation
        //         break;
        //     case UpdateType::Delete:
        //         // Handle node deletion
        //         break;
        //     case UpdateType::Connection:
        //         // Handle connection updates
        //         break;
        //     default:
        //         break;
        //     }
        // }
    }

private:
    graaf::directed_graph<std::unique_ptr<vortex::INode>, vortex::Connection> _graph;
    std::unordered_set<uintptr_t> _dirty_nodes; ///< Set of nodes that have pending property updates

    std::vector<vortex::INode*> _outputs;
    std::vector<vortex::Connection> _connections;
    uint32_t frame = 0; ///< Frame counter for rendering
};
} // namespace vortex