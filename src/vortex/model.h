#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/graph/connection.h>
#include <vortex/probe.h>
#include <atomic>
#include <unordered_set>

namespace vortex::graph {
class GraphModel
{
public:
    GraphModel() = default;

public:
    uintptr_t CreateNode(const vortex::Graphics& gfx, std::string_view node_name)
    {
        auto node = NodeFactory::CreateNode(node_name, gfx);
        if (!node) {
            vortex::error("Failed to create node: {}", node_name);
            return 0; // Return 0 if node creation failed
        }
        if (node->GetType() == NodeType::Output) {
            _outputs.push_back(node.get()); // Add to outputs if it's an output node
        }

        auto node_ptr = std::bit_cast<uintptr_t>(node.get());
        _nodes.emplace(node_ptr, std::move(node));
        return node_ptr; // Return the pointer to the created node
    }

    void RemoveNode(uintptr_t node_ptr)
    {
        auto it = _nodes.find(node_ptr);
        if (it == _nodes.end()) {
            vortex::error("Node not found in the graph: {}", node_ptr);
            return; // Node not found, nothing to remove
        }
        INode* node = std::bit_cast<INode*>(node_ptr);

        // Remove connections associated with the node
        Connection connection_to_remove{ nullptr, nullptr, 0, 0 };
        auto sinks = node->GetSinks();
        for (std::size_t i = 0; i < sinks.size(); i++) {
            auto& sink = sinks[i];
            if (!sink) {
                continue; // Skip invalid sinks
            }

            connection_to_remove.from_node = sink.source_node;
            connection_to_remove.from_index = sink.source_index;
            connection_to_remove.to_node = node;
            connection_to_remove.to_index = i;
            _connections.erase(connection_to_remove); // Remove all connections to this node
        }

        auto sources = node->GetSources();
        for (std::size_t i = 0; i < sources.size(); i++) {
            auto& source = sources[i];
            // TODO: handle sources properly
        }

        // Remove from dirty set when deleting
        _dirty_nodes.erase(node_ptr);
        if (node->GetType() == NodeType::Output) {
            if (auto output_it = std::ranges::find(_outputs, node); output_it != _outputs.end()) {
                _outputs.erase(output_it); // Remove from outputs if it's an output node
            } else {
                vortex::error("Output node not found in outputs: {}", node_ptr);
            }
        }
        _nodes.erase(it); // Remove the node from the graph
    }

    void SetNodeProperty(uintptr_t node_ptr, uint32_t index, std::string_view value, bool notify_ui = false)
    {
        if (auto* node = GetNode(node_ptr)) {
            _dirty_nodes.insert(node_ptr).second;
            node->SetProperty(index, value, notify_ui);
        }
    }

    void ConnectNodes(uintptr_t node_ptr_left, int32_t output_index, uintptr_t node_ptr_right, int32_t input_index)
    {
        auto* left_node = GetNode(node_ptr_left);
        auto* right_node = GetNode(node_ptr_right);

        if (!left_node || !right_node) {
            vortex::error("Failed to connect nodes: one or both nodes not found.");
            return; // One or both nodes not found, cannot connect
        }
        auto right_sinks = right_node->GetSinks();
        auto left_sources = left_node->GetSources();
        if (output_index < 0 || output_index >= static_cast<int32_t>(left_sources.size())) {
            vortex::error("Invalid output index {} for node {}", output_index, left_node->GetInfo());
            return; // Invalid output index
        }
        if (input_index < 0 || input_index >= static_cast<int32_t>(right_sinks.size())) {
            vortex::error("Invalid input index {} for node {}", input_index, right_node->GetInfo());
            return; // Invalid input index
        }
        vortex::info("Connecting nodes: {} (output {}) -> {} (input {})",
                     left_node->GetInfo(), output_index,
                     right_node->GetInfo(), input_index);

        // Create a connection and add it to the graph
        auto &&[it, succ] = _connections.emplace(left_node, right_node, uint32_t(output_index), uint32_t(input_index));
        if (!succ) {
            vortex::warn("Connection already exists: {} -> {} ({} -> {})",
                          left_node->GetInfo(), right_node->GetInfo(),
                          output_index, input_index);
            return; // Connection already exists
        }


        // remove any existing connection at the same input index
        auto& target_sink = right_sinks[input_index];
        if (target_sink) {
            vortex::warn("Overwriting existing connection at input index {} on node {}", input_index, right_node->GetInfo());
            _connections.erase(Connection{ target_sink.source_node, right_node, uint32_t(target_sink.source_index), uint32_t(output_index) }); // Remove existing connection
        }

        target_sink.source_node = left_node; // Set the source node for the sink
        target_sink.source_index = uint32_t(output_index); // Set the source index for the sink
    }

    void SetNodeInfo(uintptr_t node_ptr, std::string info)
    {
        if (auto* node = GetNode(node_ptr)) {
            node->SetInfo(std::move(info));
        }
    }

    void TraverseNodes(vortex::RenderProbe& probe)
    {
        // Process all pending updates before rendering
        ProcessUpdates();

        for (auto* output : _outputs) {
        }

        // probe._output_size = { 1280, 720 }; // Set output size for the probe

        // auto& cmd_list = probe._command_list;
        // auto& queue = probe._gfx.GetMainQueue();
        // cmd_list.Reset();

        // auto& output = static_cast<WindowOutput&>(*_outputs[0]);

        // uint32_t index = output._swapchain.GetCurrentIndex();
        // auto& texture = output._textures[index];

        // cmd_list.TextureBarrier(
        //         wis::TextureBarrier{
        //                 .sync_before = wis::BarrierSync::None,
        //                 .sync_after = wis::BarrierSync::RenderTarget,
        //                 .access_before = wis::ResourceAccess::NoAccess,
        //                 .access_after = wis::ResourceAccess::RenderTarget,
        //                 .state_before = wis::TextureState::Present,
        //                 .state_after = wis::TextureState::RenderTarget,
        //         },
        //         texture);

        // wis::RenderPassRenderTargetDesc rtd{
        //     .target = output._render_targets[index],
        //     .load_op = wis::LoadOperation::Clear,
        //     .store_op = wis::StoreOperation::Store,
        //     .clear_value = { 0.f, 0.f, 0.f, 1.f } // Clear to black
        // };
        // wis::RenderPassDesc rpd{
        //     .target_count = 1,
        //     .targets = &rtd,
        // };
        // cmd_list.BeginRenderPass(rpd);
        // probe._descriptor_buffer.BindBuffers(probe._gfx, cmd_list); // Bind descriptor buffers

        ////_nodes[0]->Visit(probe); // Visit the first node (ImageInput)

        // cmd_list.EndRenderPass();
        // cmd_list.TextureBarrier(
        //         wis::TextureBarrier{
        //                 .sync_before = wis::BarrierSync::RenderTarget,
        //                 .sync_after = wis::BarrierSync::None,
        //                 .access_before = wis::ResourceAccess::RenderTarget,
        //                 .access_after = wis::ResourceAccess::NoAccess,
        //                 .state_before = wis::TextureState::RenderTarget,
        //                 .state_after = wis::TextureState::Present,
        //         },
        //         texture);

        // cmd_list.Close();

        // wis::CommandListView command_lists[] = { cmd_list };
        // queue.ExecuteCommandLists(command_lists, 1);

        // output._swapchain.Present();
    }

    void PrintGraph() const
    {
        std::string out = "Graph Model:\n";
        for (const auto& [node_ptr, node] : _nodes) {
            std::format_to(std::back_inserter(out), "Node: {} (Info: {})\n", node_ptr, node->GetInfo());
        }
        for (const auto& connection : _connections) {
            std::format_to(std::back_inserter(out), "Connection: {} -> {} ({} -> {})\n",
                         connection.from_node->GetInfo(), connection.to_node->GetInfo(),
                         connection.from_index, connection.to_index);
        }
        vortex::info(out);
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

    INode* GetNode(uintptr_t node_ptr) const
    {
        auto it = _nodes.find(node_ptr);
        if (it != _nodes.end()) {
            return it->second.get(); // Return the node pointer if found
        }
        vortex::error("Node not found in the graph: {}", node_ptr);
        return nullptr; // Node not found
    }

private:
    std::unordered_map<uintptr_t, std::unique_ptr<INode>> _nodes;
    std::unordered_set<Connection> _connections; ///< Map of connections by node pointers
    std::unordered_set<uintptr_t> _dirty_nodes; ///< Set of nodes that have pending property updates

    std::vector<INode*> _outputs;
    uint32_t frame = 0; ///< Frame counter for rendering
};
} // namespace vortex::graph