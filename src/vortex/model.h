#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/graph/connection.h>
#include <vortex/graph/optimize_probe.h>
#include <vortex/probe.h>
#include <atomic>
#include <unordered_set>

namespace vortex::graph {
class GraphModel
{
public:
    GraphModel() = default;

public:
    uintptr_t CreateNode(const vortex::Graphics& gfx, std::string_view node_name, UpdateNotifier::External updater = {}, SerializedProperties values = {})
    {
        auto node = NodeFactory::CreateNode(node_name, gfx, updater, values);
        if (!node) {
            vortex::error("Failed to create node: {}", node_name);
            return 0; // Return 0 if node creation failed
        }
        if (node->GetType() == NodeType::Output) {
            _outputs.push_back(static_cast<IOutput*>(node.get())); // Add to outputs if it's an output node
        }

        // Static nodes are to be updated on changes
        UpdateIfStatic(node.get());

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
            connection_to_remove.from_index = i;
            connection_to_remove.from_node = node;
            for (const auto& target : source.targets) {
                if (!target) {
                    continue; // Skip invalid targets
                }
                // Reset the connection sink
                target.sink_node->GetSinks()[target.sink_index].Reset();

                // Reset the sink to remove the connection
                connection_to_remove.to_node = target.sink_node;
                connection_to_remove.to_index = target.sink_index;
                _connections.erase(connection_to_remove); // Remove all connections from this node
            }
        }

        // Remove from dirty set when deleting
        _dirty_nodes.erase(node);
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
            UpdateIfStatic(node); // Update the node if it is static, dynamic nodes update every frame
            node->SetProperty(index, value, notify_ui);
        }
    }

    auto GetNodeProperties(uintptr_t node_ptr) const -> std::string
    {
        if (auto* node = GetNode(node_ptr)) {
            return node->GetProperties(); // Get properties of the node
        }
        return "{}"; // Return empty string if node not found
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
        auto& target_source = left_sources[output_index];
        if (target_sink) {
            vortex::warn("Overwriting existing connection at input index {} on node {}", input_index, right_node->GetInfo());
            _connections.erase(Connection{ target_sink.source_node, right_node, uint32_t(target_sink.source_index), uint32_t(output_index) }); // Remove existing connection
        }

        target_sink.source_node = left_node; // Set the source node for the sink
        target_sink.source_index = uint32_t(output_index); // Set the source index for the sink

        target_source.targets.emplace(SourceTarget{ uint32_t(input_index), right_node }); // Add the target to the source

        UpdateIfStatic(right_node);

        // Mark nodes for optimization
        _optimize_probe.MarkNodeDirty(left_node);
        _optimize_probe.MarkNodeDirty(right_node);
        _optimization_dirty = true;
    }

    void DisconnectNodes(uintptr_t node_ptr_left, int32_t output_index, uintptr_t node_ptr_right, int32_t input_index)
    {
        auto* left_node = GetNode(node_ptr_left);
        auto* right_node = GetNode(node_ptr_right);
        if (!left_node || !right_node) {
            vortex::error("Failed to disconnect nodes: one or both nodes not found.");
            return; // One or both nodes not found, cannot disconnect
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
        vortex::info("Disconnecting nodes: {} (output {}) -> {} (input {})",
                     left_node->GetInfo(), output_index,
                     right_node->GetInfo(), input_index);
        // Remove the connection from the graph
        Connection connection_to_remove{ left_node, right_node, uint32_t(output_index), uint32_t(input_index) };
        auto it = _connections.find(connection_to_remove);
        if (it != _connections.end()) {
            _connections.erase(it); // Remove the connection if it exists
        } else {
            vortex::warn("Connection does not exist: {} -> {} ({} -> {})",
                         left_node->GetInfo(), right_node->GetInfo(),
                         output_index, input_index);
            return; // Connection does not exist
        }
        // Reset the sink and source to remove the connection
        auto& target_sink = right_sinks[input_index];
        target_sink.Reset(); // Reset the sink

        auto& target_source = left_sources[output_index];
        target_source.targets.erase(SourceTarget{ uint32_t(input_index), right_node }); // Remove the target from the source

        UpdateIfStatic(right_node); // Update the right node if it was static

        // Mark nodes for optimization
        _optimize_probe.MarkNodeDirty(left_node);
        _optimize_probe.MarkNodeDirty(right_node);
        _optimization_dirty = true;
    }

    void SetNodeInfo(uintptr_t node_ptr, std::string info)
    {
        if (auto* node = GetNode(node_ptr)) {
            node->SetInfo(std::move(info));
        }
    }

    void TraverseNodes(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
    {
        // Process all pending updates before rendering
        ProcessUpdates(gfx, probe);

        for (auto* output : _outputs) {
            output->Evaluate(gfx, probe); // Visit each output node to render
        }
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

    void OptimizeGraph()
    {
        if (_optimize_probe.HasPendingChanges() || _optimization_dirty) {
            _optimize_probe.AnalyzeGraph(_outputs);
            _optimize_probe.ApplyOptimizations();
            _optimization_dirty = false;
        }
    }

public:
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
    void ProcessUpdates(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
    {
        for (auto* node : _dirty_nodes) {
            node->Update(gfx, probe); // Update the node with the graphics context and probe
        }
    }

    void UpdateIfStatic(INode* node)
    {
        if (node->GetEvaluationStrategy() == EvaluationStrategy::Static) {
            _dirty_nodes.insert(node); // Insert into dirty nodes set for static nodes
        }
    }

private:
    std::unordered_map<uintptr_t, std::unique_ptr<INode>> _nodes;
    std::unordered_set<Connection> _connections; ///< Map of connections by node pointers
    std::unordered_set<INode*> _dirty_nodes; ///< Set of nodes that have pending property updates

    std::vector<IOutput*> _outputs;
    uint32_t frame = 0; ///< Frame counter for rendering

    OptimizeProbe _optimize_probe;
    bool _optimization_dirty = true;
};
} // namespace vortex::graph