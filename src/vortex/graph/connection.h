#pragma once
#include <vortex/util/common.h>

namespace vortex::graph {
class INode; // Forward declaration of INode
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
struct hash<vortex::graph::Connection> {
    std::size_t operator()(const vortex::graph::Connection& conn) const noexcept
    {
        std::size_t h1 = std::bit_cast<std::size_t>(conn.from_node); // pointer is unique enough
        vortex::hash_combine(h1, std::bit_cast<std::size_t>(conn.to_node));
        vortex::hash_combine(h1, std::hash<uint32_t>()(conn.from_index));
        vortex::hash_combine(h1, std::hash<uint32_t>()(conn.to_index));
        return h1;
    }
};
}