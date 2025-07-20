#pragma once
#include <memory>
#include <functional>
#include <string>
#include <source_location>
#include <vortex/util/common.h>
#include <vortex/util/lib/reflect.h>
#include <print>

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
enum class EvaluationStrategy {
    Static, // Evaluate only if properties change
    Default = Static, // Evaluate only if properties change
    Dynamic, // Evaluate every frame
    Inherited, // Evaluate based on children's evaluation strategy
};
enum class SinkType {
    RenderTexture, // Render texture sink
    RenderTarget, // Render target sink
};

// Microoptimization for update queue!
struct alignas(16) INode {
    virtual ~INode() = default;
    virtual void Visit(class RenderProbe& probe) { };
    constexpr virtual NodeType GetType() const noexcept
    {
        return NodeType::None; // Default node type
    }
    constexpr virtual EvaluationStrategy GetEvaluationStrategy() const noexcept
    {
        return EvaluationStrategy::Static; // Default evaluation strategy
    }
    virtual void SetProperty(uint32_t index, std::string_view value, bool notify = false) { }

    virtual std::string_view GetInfo() const noexcept { return ""; }
    virtual void SetInfo(std::string info) { }

};
struct IOutput : public vortex::INode {
    virtual void Enter(class RenderProbe& probe) { };
    virtual void Exit(class RenderProbe& probe) { };
};

struct Sink {
    SinkType type = SinkType::RenderTexture; // Default sink type
};
struct Source {
    virtual void Connect(Sink& sink) = 0;
    virtual void Disconnect(Sink& sink) = 0;
};

class NodeFactory
{
    // Use callback to create a node
    using CreateNodeCallback = std::unique_ptr<INode> (*)(const vortex::Graphics& gfx);

public:
    NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

public:
    static void RegisterNode(std::string_view name, CreateNodeCallback callback)
    {
        node_creators[std::string(name)] = callback;
    }
    static std::unique_ptr<INode> CreateNode(std::string_view name, const vortex::Graphics& gfx)
    {
        auto it = node_creators.find(name);
        if (it != node_creators.end()) {
            return it->second(gfx);
        }
        return nullptr;
    }

private:
    static inline std::unordered_map<std::string, CreateNodeCallback, vortex::string_hash, vortex::string_equal> node_creators;
};

// Add to your PropertiesWrapper or NodeImpl
template<typename T>
struct PropertiesWrapper : public T {
    using T::T; // Inherit constructors from T
};

template<typename CRTP, typename Properties, EvaluationStrategy strategy = EvaluationStrategy::Static, typename Base = INode>
struct NodeImpl : public Base, public PropertiesWrapper<Properties> {
    using Base::Base;
    static constexpr EvaluationStrategy evaluation_strategy = strategy;

public:
    static constexpr std::string_view name = reflect::type_name<CRTP>();
    static void RegisterNode()
    {
        auto callback = [](const vortex::Graphics& gfx) -> std::unique_ptr<INode> {
            auto node = std::make_unique<CRTP>(gfx);
            return node;
        };
        NodeFactory::RegisterNode(name, callback);
    }
    constexpr virtual NodeType GetType() const noexcept override
    {
        if constexpr (std::is_base_of_v<IOutput, CRTP>) {
            return NodeType::Output;
        } else {
            return NodeType::Filter; // Default to Filter for other nodes
        }
    }
    constexpr virtual EvaluationStrategy GetEvaluationStrategy() const noexcept override
    {
        return evaluation_strategy;
    }
    virtual void SetProperty(uint32_t index, std::string_view value, bool notify = false) override
    {
        static_cast<CRTP*>(this)->SetProperty(index, value, notify);
    }
    virtual std::string_view GetInfo() const noexcept override
    {
        return _info;
    }
    virtual void SetInfo(std::string info) override
    {
        static_cast<CRTP*>(this)->SetInfo(std::move(info));
    }

public:
    template<typename Self>
    void SetInfo(this Self&& self, std::string info) noexcept
    {
        self._info = std::format("{}: {}",
                                 reflect::type_name<Self>(),
                                 info);
    }

private:
    std::string _info = std::format("{}: {}", reflect::type_name<CRTP>(), "Unnamed");
};

template<typename CRTP, typename Properties>
using OutputImpl = NodeImpl<CRTP, Properties, EvaluationStrategy::Inherited, IOutput>;

class Connection
{
public:
    constexpr Connection(INode* from, INode* to, uint32_t from_index, uint32_t to_index)
        : from_node(from)
        , to_node(to)
        , from_index(from_index)
        , to_index(to_index)
    {
    }

    constexpr bool operator==(const Connection& other) const noexcept
    {
        return from_node == other.from_node &&
                to_node == other.to_node &&
                from_index == other.from_index &&
                to_index == other.to_index;
    }

public:
    INode* from_node = nullptr;
    INode* to_node = nullptr;
    uint32_t from_index = 0; // Index of the output on the from_node
    uint32_t to_index = 0; // Index of the input on the to_node
};
} // namespace vortex

namespace std {
template<>
struct hash<vortex::Connection> {
    std::size_t operator()(const vortex::Connection& conn) const noexcept
    {
        std::size_t h1 = std::bit_cast<std::size_t>(conn.from_node); // pointer is unique enough
        vortex::hash_combine(h1, std::bit_cast<std::size_t>(conn.to_node));
        vortex::hash_combine(h1, std::hash<uint32_t>()(conn.from_index));
        vortex::hash_combine(h1, std::hash<uint32_t>()(conn.to_index));
        return h1;
    }
};
} // namespace std