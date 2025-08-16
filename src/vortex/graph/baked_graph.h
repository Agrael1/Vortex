#pragma once
#include <vortex/graph/interfaces.h>

namespace vortex::graph {

// This node will store additional data and optimization information
class BakedNode
{
public:
    BakedNode(INode* node) noexcept
        : _node(node)
    {
    }

public:
    // Evaluate the baked node using optimized strategy
    void Evaluate(const vortex::Graphics& gfx, RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr)
    {
        if (!_node) return;
        
        auto sources = _node->GetSources();
        for (size_t i = 0; i < sources.size(); ++i) {
            auto& source = sources[i];
            
            switch (source.strategy) {
                case RenderStrategy::None:
                    // Skip this source entirely
                    continue;
                    
                case RenderStrategy::Direct:
                    // Direct rendering to provided output
                    _node->Evaluate(gfx, probe, output_info);
                    break;
                    
                case RenderStrategy::Cache: {
                    // Render to cached texture, then copy to outputs
                    //auto* cached_texture = probe.GetCachedTexture(_node, output_info->_output_size);
                    //if (cached_texture) {
                    //    // Create temporary forward desc for cached target
                    //    RenderPassForwardDesc cached_desc{
                    //        ._current_rt_view = cached_texture->render_target_view,
                    //        ._current_rt_texture = &cached_texture->texture,
                    //        ._output_size = cached_texture->size
                    //    };
                    //    
                    //    // Render to cache
                    //    _node->Evaluate(gfx, probe, &cached_desc);
                    //    
                    //    // Copy/filter from cache to actual outputs
                    //    CopyToTargets(gfx, probe, *cached_texture, source.targets, output_info->_output_size);
                    //}
                    break;
                }
                    
                case RenderStrategy::Bypass:
                    // Pass through to next node in chain
                    break;
            }
        }
    }

private:
    void CopyToTargets(const vortex::Graphics& gfx, RenderProbe& probe, 
                       //const CachedTexture& source_texture,
                       const std::unordered_set<SourceTarget, SourceTarget::Hash>& targets,
                       wis::Size2D target_size)
    {
        // Implementation for copying/filtering cached texture to multiple targets
        // This would use a blit or copy operation, potentially with scaling
        for (const auto& target : targets) {
            if (target.sink_node) {
                // Perform copy/blit operation from cached texture to target
                // This could involve format conversion, scaling, etc.
            }
        }
    }

private:
    INode* _node = nullptr; // Pointer to the node this baked node represents
};

// This will store the baked graph structure
// Once GraphModel is finalized, it can be baked into a static structure
class BakedGraph
{
public:
    BakedGraph() = default;
    
    // Add a baked node to the graph
    void AddNode(BakedNode&& node)
    {
        nodes.push_back(std::move(node));
    }

    // Clear the baked graph
    void Clear()
    {
        nodes.clear();
    }

    void Render(const vortex::Graphics& gfx, RenderProbe& probe)
    {
        for (auto& node : nodes) {
            node.Evaluate(gfx, probe); // Evaluate each baked node
        }
    }

public:
    std::vector<BakedNode> nodes; // List of baked nodes, topologically sorted
};
} // namespace vortex::graph
