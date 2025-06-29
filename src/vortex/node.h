#pragma once
#include <memory>
#include <functional>
#include <string>
#include <source_location>
#include <vortex/util/common.h>

namespace vortex {
enum class NodeType {
    None,
    Input,
    Output,
    Filter,
};
enum class NodeExecution {
    None,
    Render,
    Skip,
};
enum class NodeInput {
    INVALID = -1,
    OUTPUT_DESC = 0,

    USER_INPUT = 4096,
};

struct NodeDesc {
    NodeInput input = NodeInput::INVALID;
    void* pNext = nullptr;
};

struct OutputDesc {
    NodeInput input = NodeInput::OUTPUT_DESC;
    void* pNext = nullptr;

    wis::DataFormat format;
    wis::Size2D size;
    const char* name;
};

template<typename T>
struct NodeDescT : public NodeDesc {
    T data;
};

struct INode {
    virtual ~INode() = default;
    virtual void Visit() { };
};
struct IOutput : public vortex::INode {
};

class NodeFactory
{
    // Use callback to create a node
    using CreateNodeCallback = std::unique_ptr<INode> (*)(const vortex::Graphics& gfx, NodeDesc* initializers);

public:
    NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

public:
    static void RegisterNode(std::string_view name, CreateNodeCallback callback)
    {
        node_creators[std::string(name)] = callback;
    }
    static std::unique_ptr<INode> CreateNode(std::string_view name, const vortex::Graphics& gfx, NodeDesc* initializers)
    {
        auto it = node_creators.find(name);
        if (it != node_creators.end()) {
            return it->second(gfx, initializers);
        }
        return nullptr;
    }

private:
    static inline std::unordered_map<std::string, CreateNodeCallback, vortex::string_hash, vortex::string_equal> node_creators;
};

template<typename CRTP, typename Base = INode>
struct NodeImpl : public Base {
    using Base::Base;

public:
    static constexpr std::string_view name = reflect::type_name<CRTP>();
    static void RegisterNode()
    {
        auto callback = [](const vortex::Graphics& gfx, NodeDesc* initializers) -> std::unique_ptr<INode> {
            auto node = std::make_unique<CRTP>(gfx, initializers);
            return node;
        };
        NodeFactory::RegisterNode(name, callback);
    }
};

} // namespace vortex