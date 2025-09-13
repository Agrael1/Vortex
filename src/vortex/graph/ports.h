#pragma once
#include <limits>
#include <span>
#include <array>
#include <unordered_set>
#include <vortex/consts.h>
#include <bitset>

namespace vortex::graph {
class INode; // Forward declaration of INode

enum class SinkType {
    RenderTexture, // Render texture sink
    RenderTarget, // Render target sink
};

enum class RenderStrategy {
    None, // No rendering
    Direct, // Direct rendering to target
    Cache, // Cache output in texture cache
    Bypass, // Bypass this source entirely
};

struct Sink {
    static constexpr std::size_t dynamic_index = std::numeric_limits<std::size_t>::max(); // Invalid index for sink

    SinkType type = SinkType::RenderTexture; // Default sink type
    std::uint32_t source_index = 0; // Index of the sink in the output node
    INode* source_node = nullptr; // Pointer to the node that is the sink

    operator bool() const noexcept
    {
        return source_node != nullptr; // Check if the sink is valid
    }
    void Reset() noexcept
    {
        source_index = 0; // Reset index
        source_node = nullptr; // Reset node pointer
    }
};

template<std::size_t N>
struct SinkArray {
    std::array<Sink, N> sinks; // Array of sinks
    std::span<Sink> GetSinks() noexcept
    {
        return sinks;
    }
};

template<>
struct SinkArray<Sink::dynamic_index> {
    std::vector<Sink> sinks; // Empty array of sinks
    std::span<Sink> GetSinks() noexcept
    {
        return sinks;
    }
};

template<>
struct SinkArray<0> {
    std::span<Sink> GetSinks() noexcept
    {
        return {};
    }
};
using EmptySinks = SinkArray<0>; // Empty sink array specialization
using SinkVector = SinkArray<Sink::dynamic_index>;

struct SourceTarget {
    std::size_t sink_index = 0; // Index of the source in the node
    INode* sink_node = nullptr; // Pointer to the node that is the source

    constexpr bool operator==(const SourceTarget& other) const noexcept
    {
        return sink_node == other.sink_node && sink_index == other.sink_index; // Compare node pointers and indices
    }

    constexpr explicit operator bool() const noexcept
    {
        return sink_node != nullptr; // Check if the source is valid
    }

    constexpr void Reset() noexcept
    {
        sink_index = 0; // Reset index
        sink_node = nullptr; // Reset node pointer
    }

    // Define hash as a nested struct
    struct Hash {
        size_t operator()(const SourceTarget& target) const noexcept
        {
            std::size_t hash = std::bit_cast<std::size_t>(target.sink_node);
            return hash_combine(hash, target.sink_index);
        }
    };
};

struct Source {
    static constexpr std::size_t dynamic_index = std::numeric_limits<std::size_t>::max(); // Invalid index for sink
    std::unordered_set<SourceTarget, SourceTarget::Hash> targets; // Set of sink descriptions for this source
    std::bitset<max_outputs> rendered_outputs; // Bitset of rendered outputs (come from optimization)
};

template<std::size_t N>
struct SourceArray {
    std::array<Source, N> sources; // Array of sources
    std::span<Source> GetSources() noexcept
    {
        return sources;
    }
};
template<>
struct SourceArray<Source::dynamic_index> {
    std::vector<Source> sources; // Empty array of sources
    std::span<Source> GetSources() noexcept
    {
        return sources;
    }
};
template<>
struct SourceArray<0> {
    std::span<Source> GetSources() noexcept
    {
        return {};
    }
};
using EmptySources = SourceArray<0>; // Empty source array specialization
using SourceVector = SourceArray<Source::dynamic_index>; // Dynamic source array specialization
} // namespace vortex::graph