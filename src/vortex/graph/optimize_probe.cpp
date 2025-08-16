#include <vortex/graph/optimize_probe.h>
#include <vortex/util/log.h>
#include <algorithm>

namespace vortex::graph {

void OptimizeProbe::AnalyzeNode(INode* node) {
    if (!node || _analyzed_this_pass.contains(node)) return;
    
    _analyzed_this_pass.insert(node);
    
    auto sources = node->GetSources();
    auto& node_analyses = _source_analyses[node];
    
    for (uint32_t i = 0; i < sources.size(); ++i) {
        auto& source = sources[i];
        auto sink_requirements = CollectSinkRequirements(node, i);
        
        SourceAnalysis analysis;
        analysis.sink_requirements = std::move(sink_requirements);
        analysis.optimal_render_size = DetermineOptimalSize(analysis.sink_requirements);
        analysis.recommended_strategy = DetermineStrategy(analysis.sink_requirements);
        
        // Check if this is different from previous analysis
        auto prev_it = node_analyses.find(i);
        if (prev_it != node_analyses.end()) {
            const auto& prev = prev_it->second;
            analysis.needs_reevaluation = 
                (prev.recommended_strategy != analysis.recommended_strategy) ||
                (prev.optimal_render_size.width != analysis.optimal_render_size.width) ||
                (prev.optimal_render_size.height != analysis.optimal_render_size.height);
        } else {
            analysis.needs_reevaluation = true;
        }
        
        node_analyses[i] = std::move(analysis);
        
        vortex::trace("OptimizeProbe: Analyzed node {} source {}: strategy={}, size={:x}, sinks={}",
                     node->GetInfo(), i, 
                     static_cast<int>(node_analyses[i].recommended_strategy),
                     node_analyses[i].optimal_render_size,
                     node_analyses[i].sink_requirements.size());
    }
}

void OptimizeProbe::AnalyzeGraph(const std::vector<IOutput*>& outputs) {
    _analyzed_this_pass.clear();
    
    // Start from outputs and work backwards
    for (auto* output : outputs) {
        if (!output) continue;
        
        // Get the size requirement from the output
        wis::Size2D output_size = output->GetOutputSize();
        _node_size_cache[output] = output_size;
        
        // Traverse backwards through the graph
        std::unordered_set<INode*> visited;
        TraverseUpstream(output, visited);
    }
}

void OptimizeProbe::TraverseUpstream(INode* node, std::unordered_set<INode*>& visited) {
    if (!node || visited.contains(node)) return;
    visited.insert(node);
    
    // Analyze this node
    AnalyzeNode(node);
    
    // Continue upstream through all connected sources
    auto sinks = node->GetSinks();
    for (const auto& sink : sinks) {
        if (sink.source_node) {
            TraverseUpstream(sink.source_node, visited);
        }
    }
}

std::vector<SinkRequirement> OptimizeProbe::CollectSinkRequirements(INode* source_node, uint32_t source_index) {
    std::vector<SinkRequirement> requirements;
    
    auto sources = source_node->GetSources();
    if (source_index >= sources.size()) return requirements;
    
    const auto& source = sources[source_index];
    for (const auto& target : source.targets) {
        if (!target.sink_node) continue;
        
        SinkRequirement req;
        req.sink_node = target.sink_node;
        req.sink_index = target.sink_index;
        req.required_size = GetSinkRequiredSize(target.sink_node);
        req.required_format = wis::DataFormat::RGBA8Unorm; // Default for now
        
        requirements.push_back(req);
    }
    
    return requirements;
}

wis::Size2D OptimizeProbe::DetermineOptimalSize(const std::vector<SinkRequirement>& requirements) {
    if (requirements.empty()) return {0, 0};
    
    // Find the maximum required size
    wis::Size2D max_size = requirements[0].required_size;
    for (const auto& req : requirements) {
        max_size.width = std::max(max_size.width, req.required_size.width);
        max_size.height = std::max(max_size.height, req.required_size.height);
    }
    
    return max_size;
}

RenderStrategy OptimizeProbe::DetermineStrategy(const std::vector<SinkRequirement>& requirements) {
    if (requirements.empty()) {
        return RenderStrategy::None;
    }
    
    if (requirements.size() == 1) {
        return RenderStrategy::Direct;
    }
    
    // Multiple sinks - check if dimensions are compatible for caching
    bool compatible_dimensions = true;
    if (requirements.size() > 1) {
        const auto& base = requirements[0];
        for (size_t i = 1; i < requirements.size(); ++i) {
            if (!base.IsCompatible(requirements[i])) {
                compatible_dimensions = false;
                break;
            }
        }
    }
    
    if (compatible_dimensions) {
        return RenderStrategy::Cache;
    } else {
        // Incompatible dimensions - might need multiple evaluations
        return RenderStrategy::Direct; // Or implement multi-resolution strategy
    }
}

wis::Size2D OptimizeProbe::GetSinkRequiredSize(INode* sink_node) {
    // Check cache first
    auto cache_it = _node_size_cache.find(sink_node);
    if (cache_it != _node_size_cache.end()) {
        return cache_it->second;
    }
    
    // For output nodes, get their output size
    if (auto* output = dynamic_cast<IOutput*>(sink_node)) {
        wis::Size2D size = output->GetOutputSize();
        _node_size_cache[sink_node] = size;
        return size;
    }
    
    // For filter nodes, propagate requirements from their outputs
    auto sources = sink_node->GetSources();
    wis::Size2D max_required{0, 0};
    
    for (uint32_t i = 0; i < sources.size(); ++i) {
        const auto& source = sources[i];
        for (const auto& target : source.targets) {
            if (target.sink_node) {
                wis::Size2D target_size = GetSinkRequiredSize(target.sink_node);
                max_required.width = std::max(max_required.width, target_size.width);
                max_required.height = std::max(max_required.height, target_size.height);
            }
        }
    }
    
    _node_size_cache[sink_node] = max_required;
    return max_required;
}

void OptimizeProbe::ApplyOptimizations() {
    for (auto& [node, source_analyses] : _source_analyses) {
        auto sources = node->GetSources();
        
        for (auto& [source_index, analysis] : source_analyses) {
            if (source_index < sources.size() && analysis.needs_reevaluation) {
                // Apply the recommended strategy to the actual source
                sources[source_index].strategy = analysis.recommended_strategy;
                
                vortex::info("OptimizeProbe: Applied strategy {} to node {} source {}",
                           reflect::enum_name(analysis.recommended_strategy),
                           node->GetInfo(), source_index);
            }
        }
    }
    
    // Clear dirty flags after applying optimizations
    _dirty_nodes.clear();
    for (auto& [node, source_analyses] : _source_analyses) {
        for (auto& [source_index, analysis] : source_analyses) {
            analysis.needs_reevaluation = false;
        }
    }
}

void OptimizeProbe::MarkNodeDirty(INode* node) {
    if (node) {
        _dirty_nodes.insert(node);
        MarkUpstreamDirty(node);
    }
}

void OptimizeProbe::MarkUpstreamDirty(INode* node) {
    if (!node) return;
    
    auto sinks = node->GetSinks();
    for (const auto& sink : sinks) {
        if (sink.source_node && !_dirty_nodes.contains(sink.source_node)) {
            _dirty_nodes.insert(sink.source_node);
            MarkUpstreamDirty(sink.source_node);
        }
    }
}

const SourceAnalysis* OptimizeProbe::GetSourceAnalysis(INode* node, uint32_t source_index) const {
    auto node_it = _source_analyses.find(node);
    if (node_it == _source_analyses.end()) return nullptr;
    
    auto source_it = node_it->second.find(source_index);
    if (source_it == node_it->second.end()) return nullptr;
    
    return &source_it->second;
}

void OptimizeProbe::Clear() {
    _source_analyses.clear();
    _dirty_nodes.clear();
    _node_size_cache.clear();
    _analyzed_this_pass.clear();
}

} // namespace vortex::graph