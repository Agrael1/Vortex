#include <vortex/graph/optimize_probe.h>

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
