#include <catch2/catch_test_macros.hpp>
#include <vortex/model.h>
#include <vortex/graphics.h>
#include <vortex/nodes/node_registry.h>

static const vortex::LogOptions options{
    .name = vortex::app_log_name,
    .pattern_prefix = "vortex",
};
static const vortex::LogOptions options_gfx{
    .name = vortex::graphics_log_name,
    .pattern_prefix = "vortex.graphics",
};

class Initializer
{
public:
    static void Instance()
    {
        static Initializer instance;
        (void)instance; // Prevent unused variable warning
    }

private:
    Initializer()
    {       
        log_global.SetAsDefault();
        vortex::RegisterHardwareNodes();
    }
    vortex::Log log_global{ options };
    vortex::Log log_graphics{ options_gfx };
};
struct InitToken
{
    InitToken()
    {
        Initializer::Instance();
    }
};


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
    auto n1 = CreateNode("WindowOutput");
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("WindowOutput");
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

    auto n1 = CreateNode("WindowOutput", output_values);
    REQUIRE(n1 != 0);
    auto n2 = CreateNode("WindowOutput", output_values2);
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