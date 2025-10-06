#include <vortex/model.h>

using namespace vortex::graph;

static bool CompatiblePorts(const Source& source, const Sink& sink) noexcept
{
    // Check if the source and sink types are compatible
    return (source.type == SourceType::RenderTexture && sink.type == SinkType::RenderTexture) ||
            (source.type == SourceType::Audio && sink.type == SinkType::Audio);
}

auto vortex::graph::GraphModel::CreateNode(const vortex::Graphics& gfx,
                                           std::string_view node_name,
                                           UpdateNotifier::External updater,
                                           SerializedProperties values) -> uintptr_t
{
    auto node = NodeFactory::CreateNode(node_name, gfx, updater, values);
    if (!node) {
        vortex::error("Failed to create node: {}", node_name);
        return 0; // Return 0 if node creation failed
    }
    if (node->GetType() == NodeType::Output) {
        auto& out = _outputs.emplace_back(
                static_cast<IOutput*>(node.get())); // Add to outputs if it's an output node
        _output_scheduler.AddOutput(out);
    }

    // Static nodes are to be updated on changes
    UpdateIfStatic(node.get());

    auto node_ptr = std::bit_cast<uintptr_t>(node.get());
    _nodes.emplace(node_ptr, std::move(node));
    return node_ptr; // Return the pointer to the created node
}

void vortex::graph::GraphModel::RemoveNode(uintptr_t node_ptr)
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

        // Remove the source from the sink
        sink.source_node->GetSources()[sink.source_index].targets.erase(
                SourceTarget{ uint32_t(i), node });
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
            _output_scheduler.RemoveOutput(*output_it); // Remove from scheduler
            _outputs.erase(output_it); // Remove from outputs if it's an output node
        } else {
            vortex::error("Output node not found in outputs: {}", node_ptr);
        }
    }
    _nodes.erase(it); // Remove the node from the graph
}

void vortex::graph::GraphModel::SetNodeProperty(uintptr_t node_ptr,
                                                uint32_t index,
                                                std::string_view value,
                                                bool notify_ui)
{
    if (auto* node = GetNode(node_ptr)) {
        node->SetProperty(index, value, notify_ui);
        UpdateIfStatic(node); // Update the node if it is static, dynamic nodes update every frame
    }
}

void vortex::graph::GraphModel::SetNodePropertyByName(uintptr_t node_ptr,
                                                      std::string_view name,
                                                      std::string_view value,
                                                      bool notify_ui)
{
    if (auto* node = GetNode(node_ptr)) {
        auto [index, type] = node->GetPropertyDesc(name);
        node->SetProperty(index, value, notify_ui);
        UpdateIfStatic(node); // Update the node if it is static, dynamic nodes update every frame
    }
}

auto vortex::graph::GraphModel::GetNodeProperties(uintptr_t node_ptr) const -> std::string
{
    if (auto* node = GetNode(node_ptr)) {
        return node->GetProperties(); // Get properties of the node
    }
    return "{}"; // Return empty string if node not found
}

bool vortex::graph::GraphModel::ConnectNodes(uintptr_t node_ptr_from,
                                             int32_t output_index,
                                             uintptr_t node_ptr_to,
                                             int32_t input_index)
{
    auto* from_node = GetNode(node_ptr_from);
    auto* to_node = GetNode(node_ptr_to);
    if (!from_node || !to_node) {
        vortex::error("Failed to connect nodes: one or both nodes not found.");
        return false; // One or both nodes not found, cannot connect
    }

    auto to_sinks = to_node->GetSinks();
    auto from_sources = from_node->GetSources();
    if (output_index < 0 || output_index >= static_cast<int32_t>(from_sources.size())) {
        vortex::error("Invalid output index {} for node {}", output_index, from_node->GetInfo());
        return false; // Invalid output index
    }
    if (input_index < 0 || input_index >= static_cast<int32_t>(to_sinks.size())) {
        vortex::error("Invalid input index {} for node {}", input_index, to_node->GetInfo());
        return false; // Invalid input index
    }
    // Check port compatibility
    if (!CompatiblePorts(from_sources[output_index], to_sinks[input_index])) {
        vortex::error("Incompatible port types: {} (output {}) -> {} (input {})",
                      from_node->GetInfo(),
                      output_index,
                      to_node->GetInfo(),
                      input_index);
        return false; // Incompatible port types, cannot connect
    }
    vortex::info("Connecting nodes: {} (output {}) -> {} (input {})",
                 from_node->GetInfo(),
                 output_index,
                 to_node->GetInfo(),
                 input_index);

    // Create a connection and add it to the graph
    auto&& [it, succ] = _connections.emplace(from_node,
                                             to_node,
                                             uint32_t(output_index),
                                             uint32_t(input_index));
    if (!succ) {
        vortex::warn("Connection already exists: {} -> {} ({} -> {})",
                     from_node->GetInfo(),
                     to_node->GetInfo(),
                     output_index,
                     input_index);
        return false; // Connection already exists
    }

    // remove any existing connection at the same input index
    auto& target_sink = to_sinks[input_index];
    auto& target_source = from_sources[output_index];
    if (target_sink) {
        vortex::warn("Overwriting existing connection at input index {} on node {}",
                     input_index,
                     to_node->GetInfo());
        _connections.erase(Connection{ target_sink.source_node,
                                       to_node,
                                       uint32_t(target_sink.source_index),
                                       uint32_t(input_index) }); // Remove existing connection
        // Remove the target from the source
        auto prev_target_sources = target_sink.source_node->GetSources();
        prev_target_sources[target_sink.source_index].targets.erase(
                SourceTarget{ uint32_t(input_index), to_node });
    }

    target_sink.source_node = from_node; // Set the source node for the sink
    target_sink.source_index = uint32_t(output_index); // Set the source index for the sink

    target_source.targets.emplace(uint32_t(input_index), to_node); // Add the target to the source

    UpdateIfStatic(to_node);
    return true; // Connection successful
}

void vortex::graph::GraphModel::DisconnectNodes(uintptr_t node_ptr_from,
                                                int32_t output_index,
                                                uintptr_t node_ptr_to,
                                                int32_t input_index)
{
    auto* from_node = GetNode(node_ptr_from);
    auto* to_node = GetNode(node_ptr_to);
    if (!from_node || !to_node) {
        vortex::error("Failed to disconnect nodes: one or both nodes not found.");
        return; // One or both nodes not found, cannot disconnect
    }
    auto right_sinks = to_node->GetSinks();
    auto left_sources = from_node->GetSources();
    if (output_index < 0 || output_index >= static_cast<int32_t>(left_sources.size())) {
        vortex::error("Invalid output index {} for node {}", output_index, from_node->GetInfo());
        return; // Invalid output index
    }
    if (input_index < 0 || input_index >= static_cast<int32_t>(right_sinks.size())) {
        vortex::error("Invalid input index {} for node {}", input_index, to_node->GetInfo());
        return; // Invalid input index
    }
    vortex::info("Disconnecting nodes: {} (output {}) -> {} (input {})",
                 from_node->GetInfo(),
                 output_index,
                 to_node->GetInfo(),
                 input_index);
    // Remove the connection from the graph
    Connection connection_to_remove{ from_node,
                                     to_node,
                                     uint32_t(output_index),
                                     uint32_t(input_index) };
    auto it = _connections.find(connection_to_remove);
    if (it != _connections.end()) {
        _connections.erase(it); // Remove the connection if it exists
    } else {
        vortex::warn("Connection does not exist: {} -> {} ({} -> {})",
                     from_node->GetInfo(),
                     to_node->GetInfo(),
                     output_index,
                     input_index);
        return; // Connection does not exist
    }
    // Reset the sink and source to remove the connection
    auto& target_sink = right_sinks[input_index];
    target_sink.Reset(); // Reset the sink

    auto& target_source = left_sources[output_index];
    target_source.targets.erase(
            SourceTarget{ uint32_t(input_index), to_node }); // Remove the target from the source

    UpdateIfStatic(to_node); // Update the right node if it was static
}

void vortex::graph::GraphModel::SetNodeInfo(uintptr_t node_ptr, std::string info)
{
    if (auto* node = GetNode(node_ptr)) {
        node->SetInfo(std::move(info));
    }
}

auto vortex::graph::GraphModel::CreateAnimation(uintptr_t node_ptr) -> uintptr_t
{
    if (auto* node = GetNode(node_ptr)) {
        auto animation = _animation_manager.AddClip(node);
        return std::bit_cast<uintptr_t>(animation); // Return the pointer to the animation clip
    }
    return 0; // Return 0 if node not found
}

void vortex::graph::GraphModel::RemoveAnimation(uintptr_t animation_ptr)
{
    auto* animation = std::bit_cast<anim::AnimationClip*>(animation_ptr);
    _animation_manager.RemoveClip(animation); // Remove the animation clip
}

auto vortex::graph::GraphModel::AddPropertyTrack(uintptr_t animation_ptr,
                                                 std::string_view property_name,
                                                 std::string_view keyframes_json) -> uintptr_t
{
    auto* animation = std::bit_cast<anim::AnimationClip*>(animation_ptr);
    if (!animation) {
        vortex::error("Invalid animation pointer: {}", animation_ptr);
        return 0; // Invalid animation pointer
    }
    auto* track = animation->AddPropertyTrack(property_name);
    if (!track) {
        vortex::error("Failed to add property track for property: {}", property_name);
        return 0; // Failed to add property track
    }

    if (!keyframes_json.empty() && !track->Deserialize(keyframes_json)) {
        vortex::error("Failed to deserialize keyframes for property: {}", property_name);
        animation->RemovePropertyTrack(track); // Remove the track if deserialization failed
        return 0; // Deserialization failed
    }
    return std::bit_cast<uintptr_t>(track); // Return the pointer to the property track
}

void vortex::graph::GraphModel::AddKeyframe(uintptr_t track_ptr, std::string_view keyframes_json) 
{
    auto* track = std::bit_cast<anim::PropertyTrack*>(track_ptr);
    if (!track) {
        vortex::error("Invalid track pointer: {}", track_ptr);
        return; // Invalid track pointer
    }
    if (!track->AddKeyframe(keyframes_json)) {
        vortex::error("Failed to deserialize keyframes: {}", keyframes_json);
        return; // Deserialization failed
    }
}

void vortex::graph::GraphModel::RemoveKeyframe(uintptr_t track_ptr, uint32_t keyframe_index) 
{
    auto* track = std::bit_cast<anim::PropertyTrack*>(track_ptr);
    if (!track) {
        vortex::error("Invalid track pointer: {}", track_ptr);
        return; // Invalid track pointer
    }
    track->RemoveKeyframe(keyframe_index); // Remove the keyframe at the specified index
}

void vortex::graph::GraphModel::Play()
{
    _output_scheduler.Play();
    _animation_manager.Play(_output_scheduler.GetCurrentPTS());
}
