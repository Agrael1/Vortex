#include <catch2/catch_test_macros.hpp>
#include "mock_model.h"

class ModelTest
{
public:
    ModelTest()
    {
    }

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

TEST_CASE_METHOD(ModelTest, "CachedOptimization", "[optimize]")
{
    auto n1 = CreateNode("MockOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput");
    REQUIRE(n2 != 0);
    REQUIRE(n1 != n2); // Ensure they are different nodes

    auto n3 = CreateNode("ImageInput");
    auto node = model.GetNode(n3);
    REQUIRE(n3 != 0);
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::None);

    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);

    model.OptimizeGraph();
    REQUIRE(node != nullptr);
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::Cache);
}

TEST_CASE_METHOD(ModelTest, "CachedOptimizationFail", "[optimize]")
{
    constexpr std::pair<std::string_view, std::string_view> output_values[]{
        std::pair{ "name", "Vortex Mega Output" },
        std::pair{ "window_size", "[1080,1920]" }
    };
    constexpr std::pair<std::string_view, std::string_view> output_values2[]{
        std::pair{ "name", "Vortex Mega Output" },
        std::pair{ "window_size", "[1920,1080]" }
    };

    auto n1 = CreateNode("MockOutput", output_values);
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput", output_values2);
    REQUIRE(n2 != 0);
    REQUIRE(n1 != n2); // Ensure they are different nodes

    auto n3 = CreateNode("ImageInput");
    REQUIRE(n3 != 0);

    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);

    model.OptimizeGraph();
    auto node = model.GetNode(n3);
    REQUIRE(node != nullptr);
    // Since the outputs have different window sizes, they cannot be cached together
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::Direct);
}

TEST_CASE_METHOD(ModelTest, "CachedOptimizationWithMultipleInputs", "[optimize]")
{
    auto n1 = CreateNode("MockOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("MockOutput");
    REQUIRE(n2 != 0);
    REQUIRE(n1 != n2); // Ensure they are different nodes
    auto n3 = CreateNode("ImageInput");
    auto node = model.GetNode(n3);
    REQUIRE(n3 != 0);
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::None);
    model.ConnectNodes(n3, 0, n1, 0);
    model.ConnectNodes(n3, 0, n2, 0);
    model.OptimizeGraph();

    REQUIRE(node != nullptr);
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::Cache);



    // Add another input to the same output
    auto n4 = CreateNode("ImageInput");
    model.ConnectNodes(n4, 0, n1, 0); // This should remove the connection from n3 to n1, so no more caching
    
    model.OptimizeGraph();
    REQUIRE(node->GetSources()[0].strategy == vortex::graph::RenderStrategy::Direct);
}