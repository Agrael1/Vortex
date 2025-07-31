#pragma once
#include <vortex/graph/node_factory.h>
#include <vortex/graph/ports.h>
#include <vortex/util/reflection.h>

namespace vortex {
class Graphics; // Forward declaration of Graphics class
class RenderProbe; // Forward declaration of RenderProbe class
struct RenderPassForwardDesc; // Forward declaration of RenderPassForwardDesc struct
} // namespace vortex

namespace vortex::graph {
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

// Microoptimization for update queue!
struct alignas(16) INode {
    virtual ~INode() = default;
    virtual void Update(const vortex::Graphics& gfx, RenderProbe& probe) { };
    virtual void Evaluate(const vortex::Graphics& gfx, RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) { };
    virtual void SetPropertyUpdateNotifier(UpdateNotifier notifier) { }
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

    virtual std::span<Sink> GetSinks() noexcept
    {
        return {}; // Default implementation returns empty span
    }
    virtual std::span<Source> GetSources() noexcept
    {
        return {}; // Default implementation returns empty span
    }
};
struct IOutput : public INode {
    virtual bool IsReady() const noexcept = 0; ///< Check if the output is ready for rendering
};

// Factory for creating nodes
template<typename CRTP,
         typename Properties,
         std::size_t sinks = 0,
         std::size_t sources = 0,
         EvaluationStrategy strategy = EvaluationStrategy::Static,
         typename Base = INode>
struct NodeImpl : public Base, public Properties {
    static constexpr EvaluationStrategy evaluation_strategy = strategy;
    using Sinks = SinkArray<sinks>;
    using Sources = SourceArray<sources>;
    using ImplClass = NodeImpl<CRTP, Properties, sinks, sources, strategy, Base>;

public:
    NodeImpl(SerializedProperties properties = {})
    {
        Properties::Deserialize(properties, false); // Deserialize properties from the provided span, don't notify since there is no notifier set yet
    }

public:
    static void RegisterNode()
    {
        auto callback = [](const vortex::Graphics& gfx, UpdateNotifier::External updater = {}, SerializedProperties values = {}) -> std::unique_ptr<INode> {
            auto node = std::make_unique<CRTP>(gfx, values);
            node->SetPropertyUpdateNotifier({ std::bit_cast<uintptr_t>(node.get()), std::move(updater) });
            node->SetInitialized(); // Mark the node as initialized
            return node;
        };
        NodeFactory::RegisterNode(reflect::type_name<CRTP>(), callback);
    }
    virtual void SetPropertyUpdateNotifier(UpdateNotifier notifier) override
    {
        static_cast<CRTP*>(this)->notifier = std::move(notifier);
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
        static_cast<CRTP*>(this)->SetPropertyStub(index, value, notify);
    }
    virtual std::string_view GetInfo() const noexcept override
    {
        return _info;
    }
    virtual void SetInfo(std::string info) override
    {
        static_cast<CRTP*>(this)->SetInfoStub(std::move(info));
    }
    virtual std::span<Sink> GetSinks() noexcept override
    {
        return _sinks.GetSinks();
    }
    virtual std::span<Source> GetSources() noexcept override
    {
        return _sources.GetSources();
    }

    // Called from the node factory to initialize the node
    void SetInitialized() noexcept
    {
        _initialized = true;
    }
    bool IsInitialized() const noexcept
    {
        return _initialized; // Check if the node is initialized
    }

public:
    template<typename Self>
    void SetInfoStub(this Self&& self, std::string info) noexcept
    {
        self._info = std::format("{}: {}",
                                 reflect::type_name<Self>(),
                                 info);
    }

private:
    bool _initialized = false; ///< Flag to check if the node is initialized
    std::string _info = std::format("{}: {}", reflect::type_name<CRTP>(), "Unnamed");

protected:
    Sinks _sinks; ///< Sinks for this node
    Sources _sources; ///< Sources for this node
};

template<typename CRTP, typename Properties, std::size_t sinks = 1>
using OutputImpl = NodeImpl<CRTP, Properties, sinks, 0, EvaluationStrategy::Inherited, IOutput>;

template<typename CRTP, typename Properties, std::size_t sinks = 0, std::size_t sources = 0, EvaluationStrategy strategy = EvaluationStrategy::Inherited>
using FilterImpl = NodeImpl<CRTP, Properties, sinks, sources, strategy, INode>;
} // namespace vortex::graph