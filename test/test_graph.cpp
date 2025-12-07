#include <catch2/catch_test_macros.hpp>
#include "mock_model.h"
#include <tuple>
#include <vector>
#include <string>

class GraphTest
{
public:
    auto CreateNode(std::string_view name, vortex::SerializedProperties values = {})
    {
        return model.CreateNode(gfx, name, {}, values);
    }

public:
    InitToken initializer;
    vortex::Graphics gfx{ true };
    vortex::graph::GraphModel model;
};

TEST_CASE_METHOD(GraphTest, "Connection.OverwriteConnection", "[connect]")
{
    auto n1 = CreateNode("MockOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput");
    REQUIRE(n2 != 0);
    REQUIRE(n1 != n2); // Ensure they are different nodes

    auto n3 = CreateNode("ImageInput");
    auto node = model.GetNode(n3);
    auto sources = node->GetSources();
    REQUIRE(n3 != 0);
    REQUIRE(sources[0].targets.size() == 0);

    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);
    REQUIRE(sources[0].targets.size() == 2); // Should have two targets now

    auto n4 = CreateNode("ImageInput");
    REQUIRE(n4 != 0);
    model.ConnectNodes(n4, 0, n1, 0); 

    REQUIRE(sources[0].targets.size() == 1);
    // Should have one target now, as n1 was disconnected
    auto& target = *sources[0].targets.begin();
    REQUIRE(target.sink_node == std::bit_cast<vortex::graph::INode*>(n2));
}

TEST_CASE_METHOD(GraphTest, "Connection.RemoveConnection", "[connect]")
{
    auto n1 = CreateNode("MockOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput");
    REQUIRE(n2 != 0);
    REQUIRE(n1 != n2); // Ensure they are different nodes

    auto n3 = CreateNode("ImageInput");
    auto node = model.GetNode(n3);
    auto sources = node->GetSources();
    REQUIRE(n3 != 0);
    REQUIRE(sources[0].targets.size() == 0);

    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);
    REQUIRE(sources[0].targets.size() == 2); // Should have two targets now

    model.DisconnectNodes(n3, 0, n1, 0);
    REQUIRE(sources[0].targets.size() == 1); // Should have one target now
    auto& target = *sources[0].targets.begin();
    REQUIRE(target.sink_node == std::bit_cast<vortex::graph::INode*>(n2));

    auto node_n1 = model.GetNode(n1);
    auto sinks_n1 = node_n1->GetSinks();
    REQUIRE(sinks_n1[0].source_node == nullptr); // n1 should have no connections now
}

TEST_CASE_METHOD(GraphTest, "Connection.DeleteNodeRemovesAllConnections", "[connect]")
{
    auto n1 = CreateNode("MockOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput");
    REQUIRE(n2 != 0);
    auto n3 = CreateNode("ImageInput");
    REQUIRE(n3 != 0);

    // Connect n3 to both n1 and n2
    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);

    // Verify connections exist
    auto node3 = model.GetNode(n3);
    auto sources = node3->GetSources();
    REQUIRE(sources[0].targets.size() == 2);

    // Delete n1
    model.RemoveNode(n1);

    // Verify n1's connection was removed from n3
    REQUIRE(sources[0].targets.size() == 1);
    auto& remaining_target = *sources[0].targets.begin();
    REQUIRE(remaining_target.sink_node == std::bit_cast<vortex::graph::INode*>(n2));

    // Verify n1 no longer exists in the model
    auto deleted_node = model.GetNode(n1);
    REQUIRE(deleted_node == nullptr);
}

TEST_CASE_METHOD(GraphTest, "Edge callbacks fire on connect/disconnect", "[connect][events]")
{
    std::vector<std::tuple<uintptr_t, int32_t, uintptr_t, int32_t, std::string>> events;

    model.SetEdgeEventCallbacks(
            [&](uintptr_t source_ptr,
                int32_t source_slot,
                uintptr_t target_ptr,
                int32_t target_slot) {
                events.emplace_back(source_ptr, source_slot, target_ptr, target_slot, "connect");
            },
            [&](uintptr_t source_ptr,
                int32_t source_slot,
                uintptr_t target_ptr,
                int32_t target_slot) {
                events.emplace_back(source_ptr, source_slot, target_ptr, target_slot, "disconnect");
            });

    auto src = CreateNode("ImageInput");
    auto dst = CreateNode("MockOutput");
    REQUIRE(src != 0);
    REQUIRE(dst != 0);

    REQUIRE(model.ConnectNodes(src, 0, dst, 0));
    REQUIRE(events.size() == 1);
    auto [connect_src, connect_src_slot, connect_dst, connect_dst_slot, connect_kind] = events.front();
    REQUIRE(connect_kind == "connect");
    REQUIRE(connect_src == src);
    REQUIRE(connect_dst == dst);
    REQUIRE(connect_src_slot == 0);
    REQUIRE(connect_dst_slot == 0);

    REQUIRE(model.DisconnectNodes(src, 0, dst, 0));
    REQUIRE(events.size() == 2);
    auto [disconnect_src,
          disconnect_src_slot,
          disconnect_dst,
          disconnect_dst_slot,
          disconnect_kind] = events.back();
    REQUIRE(disconnect_kind == "disconnect");
    REQUIRE(disconnect_src == src);
    REQUIRE(disconnect_dst == dst);
    REQUIRE(disconnect_src_slot == 0);
    REQUIRE(disconnect_dst_slot == 0);
}
