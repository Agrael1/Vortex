#include <vortex/graph/optimize_probe.h>
//#include <vortex/util/log.h>
//#include <algorithm>
//#include <unordered_set>

//namespace vortex::graph {
//
//void OptimizeProbe::AnalyzeActiveOutputs(std::span<IOutput*> all_outputs) {
//    _active_outputs.clear();
//    _shared_cache_nodes.clear();
//    
//    // 1. Filter to only active outputs
//    for (auto* output : all_outputs) {
//        if (output && output->IsReady()) {
//            _active_outputs.push_back(output);
//        }
//    }
//    
//    if (_active_outputs.empty()) return;
//    
//    // 2. Find shared upstream nodes only among active outputs
//    std::unordered_map<INode*, uint32_t> node_usage_count;
//    
//    for (auto* output : _active_outputs) {
//        std::unordered_set<INode*> upstream_nodes;
//        CollectUpstreamNodes(output, upstream_nodes);
//        
//        for (auto* node : upstream_nodes) {
//            node_usage_count[node]++;
//        }
//    }
//    
//    // 3. Mark nodes used by multiple active outputs as cacheable
//    for (const auto& [node, count] : node_usage_count) {
//        if (count > 1) {
//            _shared_cache_nodes.insert(node);
//            MarkNodeForCaching(node);
//        }
//    }
//    
//    vortex::info("OptimizeProbe: Found {} active outputs, {} shared cache nodes", 
//                _active_outputs.size(), _shared_cache_nodes.size());
//}
//
//void OptimizeProbe::CollectUpstreamNodes(INode* node, std::unordered_set<INode*>& upstream_nodes) {
//    if (!node || upstream_nodes.contains(node)) return;
//    
//    upstream_nodes.insert(node);
//    
//    auto sinks = node->GetSinks();
//    for (const auto& sink : sinks) {
//        if (sink.source_node) {
//            CollectUpstreamNodes(sink.source_node, upstream_nodes);
//        }
//    }
//}
//
//void OptimizeProbe::MarkNodeForCaching(INode* node) {
//    auto sources = node->GetSources();
//    for (auto& source : sources) {
//        // Simple rule: cache if used by multiple outputs
//        source.strategy = RenderStrategy::Cache;
//    }
//}
//
//void OptimizeProbe::OptimizeForActiveOutputs() {
//    // Simple three-step process:
//    
//    // 1. Clear previous optimizations
//    ClearOptimizations();
//    
//    // 2. Analyze only active outputs
//    AnalyzeActiveOutputs(_current_outputs);
//    
//    // 3. Apply caching to shared nodes
//    ApplySharedNodeCaching();
//    
//    vortex::trace("OptimizeProbe: Optimization complete for {} active outputs", 
//                  _active_outputs.size());
//}
//
//void OptimizeProbe::ClearOptimizations() {
//    // Reset all nodes to direct rendering
//    for (auto* output : _current_outputs) {
//        if (!output) continue;
//        
//        std::unordered_set<INode*> visited;
//        ResetUpstreamStrategies(output, visited);
//    }
//}
//
//void OptimizeProbe::ResetUpstreamStrategies(INode* node, std::unordered_set<INode*>& visited) {
//    if (!node || visited.contains(node)) return;
//    visited.insert(node);
//    
//    auto sources = node->GetSources();
//    for (auto& source : sources) {
//        source.strategy = RenderStrategy::Direct;
//    }
//    
//    auto sinks = node->GetSinks();
//    for (const auto& sink : sinks) {
//        if (sink.source_node) {
//            ResetUpstreamStrategies(sink.source_node, visited);
//        }
//    }
//}
//
//void OptimizeProbe::ApplySharedNodeCaching() {
//    for (auto* node : _shared_cache_nodes) {
//        MarkNodeForCaching(node);
//        
//        vortex::trace("OptimizeProbe: Enabled caching for shared node {}", 
//                     node->GetInfo());
//    }
//}
//
//// Simplified interface - just pass active outputs
//void OptimizeProbe::UpdateActiveOutputs(std::span<IOutput*> outputs) {
//    _current_outputs = std::vector<IOutput*>(outputs.begin(), outputs.end());
//    OptimizeForActiveOutputs();
//}
//
//// Check if a node should be cached (simple lookup)
//bool OptimizeProbe::ShouldCacheNode(INode* node) const {
//    return _shared_cache_nodes.contains(node);
//}
//
//// Get render order for active outputs (much simpler)
//std::span<const IOutput*> OptimizeProbe::GetActiveOutputs() const {
//    return _active_outputs;
//}

// Lightweight dirty marking - only for shared cache nodes
void vortex::graph::OptimizeProbe::MarkNodeDirty(INode* node)
{
    auto [it, succ] = _dirty_nodes.emplace(node);
    if (succ) {
        vortex::trace("OptimizeProbe: Marked node {} as dirty", node->GetInfo());
    } else {
        vortex::trace("OptimizeProbe: Node {} already marked dirty", node->GetInfo());
    }
}

void vortex::graph::OptimizeProbe::PropagateOutputs(std::span<IOutput*> outputs)
{
    if (outputs.empty()) {
        vortex::warn("OptimizeProbe: No outputs provided for optimization");
        return; // Nothing to optimize
    }
    
    
}

//void OptimizeProbe::InvalidateDirtyCaches() {
//    for (auto* node : _dirty_cache_nodes) {
//        // Invalidate cached results for this node
//        auto sources = node->GetSources();
//        for (auto& source : sources) {
//            if (source.strategy == RenderStrategy::Cache) {
//                // Mark cache as invalid - will be re-rendered next frame
//                source.cached_texture_valid = false;
//            }
//        }
//    }
//    _dirty_cache_nodes.clear();
//}
//
//void OptimizeProbe::Clear() {
//    _active_outputs.clear();
//    _shared_cache_nodes.clear();
//    _dirty_cache_nodes.clear();
//    _current_outputs.clear();
//}
//
//} // namespace vortex::graph