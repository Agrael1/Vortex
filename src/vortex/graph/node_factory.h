#pragma once
#include <unordered_map>
#include <vortex/util/common.h>
#include <vortex/util/reflection.h>
#include <memory>

namespace vortex {
class Graphics;
}

namespace vortex::graph {
class INode; // Forward declaration of INode
class NodeFactory
{
    // Use callback to create a node
    using CreateNodeCallback = std::unique_ptr<INode> (*)(const vortex::Graphics& gfx, UpdateNotifier::External updater);

public:
    NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

public:
    static void RegisterNode(std::string_view name, CreateNodeCallback callback)
    {
        node_creators[std::string(name)] = callback;
    }
    static std::unique_ptr<INode> CreateNode(std::string_view name, const vortex::Graphics& gfx, UpdateNotifier::External updater = {})
    {
        auto it = node_creators.find(name);
        if (it != node_creators.end()) {
            return it->second(gfx, updater);
        }
        return nullptr;
    }

private:
    static inline std::unordered_map<std::string, CreateNodeCallback, vortex::string_hash, vortex::string_equal> node_creators;
};
} // namespace vortex::graph