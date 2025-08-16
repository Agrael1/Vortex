#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/graph/ports.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace vortex::graph {

// Information about a sink's requirements
struct SinkRequirement {
    wis::Size2D required_size;
    wis::DataFormat required_format = wis::DataFormat::RGBA8Unorm;
    INode* sink_node = nullptr;
    uint32_t sink_index = 0;
    
    bool IsCompatible(const SinkRequirement& other) const noexcept {
        return required_format == other.required_format &&
               required_size.width <= other.required_size.width &&
               required_size.height <= other.required_size.height;
    }
};

// Analysis result for a source output
struct SourceAnalysis {
    RenderStrategy recommended_strategy = RenderStrategy::Direct;
    wis::Size2D optimal_render_size{0, 0};
    wis::DataFormat optimal_format = wis::DataFormat::RGBA8Unorm;
    std::vector<SinkRequirement> sink_requirements;
    bool needs_reevaluation = false;
    
    bool ShouldCache() const noexcept {
        return sink_requirements.size() >= 2 && recommended_strategy == RenderStrategy::Cache;
    }
    
    bool HasCompatibleDimensions() const noexcept {
        if (sink_requirements.empty()) return true;
        
        auto& base = sink_requirements[0];
        return std::all_of(sink_requirements.begin() + 1, sink_requirements.end(),
                          [&base](const auto& req) { return base.IsCompatible(req); });
    }
};

class OptimizeProbe {
public:
    OptimizeProbe() = default;
    
    // Analyze a single node and determine optimal strategies for its sources
    void AnalyzeNode(INode* node);
    
    // Analyze the entire graph starting from output nodes
    void AnalyzeGraph(const std::vector<IOutput*>& outputs);

    // Traverse upstream from a node to mark all connected nodes for re-analysis
    void TraverseUpstream(INode* node, std::unordered_set<INode*>& visited);
    
    // Mark a node as needing re-analysis (for incremental updates)
    void MarkNodeDirty(INode* node);
    
    // Apply optimized strategies to the graph
    void ApplyOptimizations();
    
    // Get analysis result for a specific node's source
    const SourceAnalysis* GetSourceAnalysis(INode* node, uint32_t source_index) const;
    
    // Check if any changes were detected that require re-evaluation
    bool HasPendingChanges() const noexcept { return !_dirty_nodes.empty(); }
    
    // Clear all analysis data
    void Clear();
    
private:
    // Collect requirements from all sinks connected to a source
    std::vector<SinkRequirement> CollectSinkRequirements(INode* source_node, uint32_t source_index);
    
    // Determine the optimal render size for a set of sink requirements
    wis::Size2D DetermineOptimalSize(const std::vector<SinkRequirement>& requirements);
    
    // Determine the best strategy based on sink analysis
    RenderStrategy DetermineStrategy(const std::vector<SinkRequirement>& requirements);
    
    // Get the size requirement for a specific sink node
    wis::Size2D GetSinkRequiredSize(INode* sink_node);
    
    // Traverse upstream to mark nodes for re-analysis
    void MarkUpstreamDirty(INode* node);

private:
    // Map of node -> source_index -> analysis
    std::unordered_map<INode*, std::unordered_map<uint32_t, SourceAnalysis>> _source_analyses;
    
    // Nodes that need re-analysis
    std::unordered_set<INode*> _dirty_nodes;
    
    // Cache of node size requirements
    std::unordered_map<INode*, wis::Size2D> _node_size_cache;
    
    // Track which nodes have been analyzed this pass
    std::unordered_set<INode*> _analyzed_this_pass;
};

} // namespace vortex::graph