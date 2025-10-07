#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/graph/connection.h>
#include <vortex/graph/output_scheduler.h>
#include <vortex/anim/animation.h>
#include <vortex/sync/timeline.h>
#include <vortex/probe.h>
#include <atomic>
#include <unordered_set>
#include <algorithm>

namespace vortex::graph {
class GraphModel
{
public:
    GraphModel() = default;

public: // Model API
    auto CreateNode(const vortex::Graphics& gfx,
                    std::string_view node_name,
                    UpdateNotifier::External updater = {},
                    SerializedProperties values = {}) -> uintptr_t;
    void RemoveNode(uintptr_t node_ptr);
    void SetNodeProperty(uintptr_t node_ptr,
                         uint32_t index,
                         std::string_view value,
                         bool notify_ui = false);
    void SetNodePropertyByName(uintptr_t node_ptr,
                               std::string_view name,
                               std::string_view value,
                               bool notify_ui = false);
    auto GetNodeProperties(uintptr_t node_ptr) const -> std::string;
    bool ConnectNodes(uintptr_t node_ptr_from,
                      int32_t output_index,
                      uintptr_t node_ptr_to,
                      int32_t input_index);
    void DisconnectNodes(uintptr_t node_ptr_from,
                         int32_t output_index,
                         uintptr_t node_ptr_to,
                         int32_t input_index);
    void SetNodeInfo(uintptr_t node_ptr, std::string info);
    auto CreateAnimation(uintptr_t node_ptr) -> uintptr_t;
    void RemoveAnimation(uintptr_t animation_ptr);
    auto AddPropertyTrack(uintptr_t animation_ptr,
                          std::string_view property_name,
                          std::string_view keyframes_json) -> uintptr_t;
    void AddKeyframe(uintptr_t track_ptr, std::string_view keyframes_json);
    void RemoveKeyframe(uintptr_t track_ptr, uint32_t keyframe_index);
    void Play();
    void Stop();

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

    void TraverseNodes(const vortex::Graphics& gfx)
    {
        // Process all pending updates before rendering
        ProcessUpdates(gfx);

        // Early out if no nodes or outputs or not playing
        if (_outputs.empty() || !_playing) {
            return; // No nodes or outputs to process
        }

        // Get the next output to evaluate based on scheduling
        auto [output, pts] = _output_scheduler.GetNextReadyOutput();
        if (!output) {
            return; // No output is ready to be evaluated this frame
        }

        // Evaluate properties based on the current timeline
        _animation_manager.EvaluateAtPTS(pts);

        // Evaluate the output node
        output->Evaluate(gfx, pts);
    }

    void PrintGraph() const
    {
        std::string out = "Graph Model:\n";
        for (const auto& [node_ptr, node] : _nodes) {
            std::format_to(std::back_inserter(out),
                           "Node: {} (Info: {})\n",
                           node_ptr,
                           node->GetInfo());
        }
        for (const auto& connection : _connections) {
            std::format_to(std::back_inserter(out),
                           "Connection: {} -> {} ({} -> {})\n",
                           connection.from_node->GetInfo(),
                           connection.to_node->GetInfo(),
                           connection.from_index,
                           connection.to_index);
        }
        vortex::info(out);
    }

    std::span<IOutput*> GetOutputs() noexcept
    {
        return _outputs; // Return a span of outputs
    }

    // Get the output scheduler for external access
    const OutputScheduler& GetOutputScheduler() const noexcept { return _output_scheduler; }

    anim::AnimationSystem& GetAnimationManager() noexcept { return _animation_manager; }

private:
    void ProcessUpdates(const vortex::Graphics& gfx)
    {
        for (auto* node : _dirty_nodes) {
            node->Update(gfx); // Update the node with the graphics context
        }
    }

    void UpdateIfStatic(INode* node)
    {
        if (node->GetEvaluationStrategy() == EvaluationStrategy::Static) {
            _dirty_nodes.insert(node); // Insert into dirty nodes set for static nodes
        }
    }

    // Improved compatibility-based comparison for output sorting
    static bool CompareBySizeCompatibility(const IOutput* a, const IOutput* b)
    {
        auto a_size = a->GetOutputSize();
        auto b_size = b->GetOutputSize();

        // First, group by aspect ratio (primary grouping criterion)
        float aspect_a = static_cast<float>(a_size.width) / a_size.height;
        float aspect_b = static_cast<float>(b_size.width) / b_size.height;

        // Calculate aspect ratio difference for grouping
        float aspect_diff_a = std::abs(aspect_a - 1.0f); // Distance from square (1:1)
        float aspect_diff_b = std::abs(aspect_b - 1.0f);

        // Group similar aspect ratios together (within compatibility tolerance)
        const float aspect_tolerance = 0.1f;
        bool a_near_square = aspect_diff_a <= aspect_tolerance;
        bool b_near_square = aspect_diff_b <= aspect_tolerance;

        if (a_near_square != b_near_square) {
            return a_near_square > b_near_square; // Square-like outputs first
        }

        // Within the same aspect ratio group, sort by scale factor
        // Calculate base scale (using smaller dimension as reference)
        uint32_t a_min_dim = std::min(a_size.width, a_size.height);
        uint32_t b_min_dim = std::min(b_size.width, b_size.height);

        // Group by scale ranges: [1-512], [513-1024], [1025-2048], [2049+]
        auto get_scale_group = [](uint32_t min_dim) -> int {
            if (min_dim <= 512) {
                return 0;
            }
            if (min_dim <= 1024) {
                return 1;
            }
            if (min_dim <= 2048) {
                return 2;
            }
            return 3;
        };

        int scale_group_a = get_scale_group(a_min_dim);
        int scale_group_b = get_scale_group(b_min_dim);

        if (scale_group_a != scale_group_b) {
            return scale_group_a > scale_group_b; // Larger scale groups first
        }

        // Within the same scale group, sort by total pixels (larger first)
        uint64_t a_pixels = static_cast<uint64_t>(a_size.width) * a_size.height;
        uint64_t b_pixels = static_cast<uint64_t>(b_size.width) * b_size.height;

        if (a_pixels != b_pixels) {
            return a_pixels > b_pixels;
        }

        // Final tie-breaker: width then height
        if (a_size.width != b_size.width) {
            return a_size.width > b_size.width;
        }

        return a_size.height > b_size.height;
    }

public:
    // Helper method to check if two outputs are size-compatible for caching
    static bool AreSizeCompatible(const IOutput* a, const IOutput* b) noexcept
    {
        auto a_size = a->GetOutputSize();
        auto b_size = b->GetOutputSize();

        // Check aspect ratio compatibility (within 10% tolerance)
        float aspect_a = static_cast<float>(a_size.width) / a_size.height;
        float aspect_b = static_cast<float>(b_size.width) / b_size.height;
        float aspect_diff = std::abs(aspect_a - aspect_b) / std::max(aspect_a, aspect_b);

        if (aspect_diff > 0.1f) {
            return false; // Incompatible aspect ratios
        }

        // Check scale factor (max 2x scaling)
        float scale_x = static_cast<float>(std::max(a_size.width, b_size.width)) /
                std::min(a_size.width, b_size.width);
        float scale_y = static_cast<float>(std::max(a_size.height, b_size.height)) /
                std::min(a_size.height, b_size.height);

        return std::max(scale_x, scale_y) <= 2.0f; // Compatible scaling
    }

private:
    std::unordered_map<uintptr_t, std::unique_ptr<INode>> _nodes;
    std::unordered_set<Connection> _connections; ///< Map of connections by node pointers
    std::unordered_set<INode*> _dirty_nodes; ///< Set of nodes that have pending property updates

    std::vector<IOutput*> _outputs;
    OutputScheduler _output_scheduler; ///< Frame-rate aware output scheduler
    anim::AnimationSystem _animation_manager; ///< Animation manager for property animations
    bool _playing = false; ///< Whether the model is currently playing
};
} // namespace vortex::graph