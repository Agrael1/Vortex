#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/util/common.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <span>

namespace vortex::graph {

class OptimizeProbe
{
public:
    //    // Simplified interface
    //    void UpdateActiveOutputs(std::span<IOutput*> outputs);
    //    std::span<const IOutput*> GetActiveOutputs() const;
    //
    //    bool ShouldCacheNode(INode* node) const;
    void MarkNodeDirty(INode* node);
    void PropagateOutputs(std::span<IOutput*> outputs);
    //
    //    void Clear();
    //
    // private:
    //    // Core optimization logic
    //    void OptimizeForActiveOutputs();
    //    void AnalyzeActiveOutputs(std::span<IOutput*> all_outputs);
    //    void ClearOptimizations();
    //    void ApplySharedNodeCaching();
    //
    //    // Helper methods
    //    void CollectUpstreamNodes(INode* node, std::unordered_set<INode*>& upstream_nodes);
    //    void MarkNodeForCaching(INode* node);
    //    void ResetUpstreamStrategies(INode* node, std::unordered_set<INode*>& visited);
    //
private:
    std::unordered_set<INode*> _dirty_nodes; // Cache nodes that need invalidation
    //    // Simplified state
    //    std::vector<IOutput*> _current_outputs;
    //    std::vector<IOutput*> _active_outputs;           // Only ready outputs
    //    std::unordered_set<INode*> _shared_cache_nodes;  // Nodes used by multiple active outputs
};

} // namespace vortex::graph