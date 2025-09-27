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
    void MarkNodeDirty(INode* node);
    void PropagateOutputs(std::span<IOutput*> outputs);

private:
    std::unordered_set<INode*> _dirty_nodes; // Cache nodes that need invalidation
};

} // namespace vortex::graph