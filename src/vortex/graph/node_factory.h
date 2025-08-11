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
    using CreateNodeCallback = std::unique_ptr<INode> (*)(const vortex::Graphics& gfx, UpdateNotifier::External updater, SerializedProperties values);

public:
    NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

public:
    static void RegisterNode(std::string_view name, CreateNodeCallback callback, StaticNodeInfo info)
    {
        auto&& [it, succ] = node_creators.emplace(std::string(name), callback);
        static_node_info[it->first] = info; // Store static node info
    }
    static std::unique_ptr<INode> CreateNode(std::string_view name, const vortex::Graphics& gfx, UpdateNotifier::External updater = {}, SerializedProperties values = {})
    {
        auto it = node_creators.find(name);
        if (it != node_creators.end()) {
            return it->second(gfx, updater, values);
        }
        return nullptr;
    }
    static const auto& GetNodesInfo() noexcept
    {
        return static_node_info;
    }

private:
    static inline std::unordered_map<std::string, CreateNodeCallback, vortex::string_hash, vortex::string_equal> node_creators;
    static inline std::unordered_map<std::string_view, StaticNodeInfo, vortex::string_hash, vortex::string_equal> static_node_info;
};
} // namespace vortex::graph