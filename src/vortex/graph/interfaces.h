#pragma once
#include <vortex/graph/node_factory.h>
#include <vortex/graph/ports.h>

namespace vortex {
class Graphics; // Forward declaration of Graphics class
class RenderProbe; // Forward declaration of RenderProbe class
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
    virtual void Visit(RenderProbe& probe) { };
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
};

// Factory for creating nodes

template<typename CRTP,
         typename Properties,
         std::size_t sinks = 0,
         std::size_t sources = 0,
         EvaluationStrategy strategy = EvaluationStrategy::Static,
         typename Base = INode>
struct NodeImpl : public Base, public Properties {
    using Base::Base;
    static constexpr EvaluationStrategy evaluation_strategy = strategy;
    using Sinks = SinkArray<sinks>;
    using Sources = SourceArray<sources>;

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

public:
    template<typename Self>
    void SetInfoStub(this Self&& self, std::string info) noexcept
    {
        self._info = std::format("{}: {}",
                                 reflect::type_name<Self>(),
                                 info);
    }

private:
    std::string _info = std::format("{}: {}", reflect::type_name<CRTP>(), "Unnamed");

protected:
    Sinks _sinks; ///< Sinks for this node
    Sources _sources; ///< Sources for this node
};

template<typename CRTP, typename Properties, std::size_t sinks = 1>
using OutputImpl = NodeImpl<CRTP, Properties, sinks, 0, EvaluationStrategy::Inherited, IOutput>;

} // namespace vortex::graph