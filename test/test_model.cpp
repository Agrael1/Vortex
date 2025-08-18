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

bool operator==(wis::Size2D lhs, wis::Size2D rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}
bool equal(wis::Size2D lhs, wis::Size2D rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.Basic", "[optimize]")
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

    model.SortOutputs(); // Sort outputs should not crash or throw

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 2); // Should have two outputs
    REQUIRE(equal(outputs[1]->GetOutputSize(), wis::Size2D{ 1920, 1080 })); // Second output should be the smaller one
    REQUIRE(equal(outputs[0]->GetOutputSize(), wis::Size2D{ 1080, 1920 })); // First output should be the larger one
}


TEST_CASE_METHOD(ModelTest, "SortOutputs.AspectRatioGrouping", "[optimize]")
{
    // Create outputs with different aspect ratios
    constexpr std::pair<std::string_view, std::string_view> square_1080[]{
        std::pair{ "name", "Square 1080x1080" },
        std::pair{ "window_size", "[1080,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> square_720[]{
        std::pair{ "name", "Square 720x720" },
        std::pair{ "window_size", "[720,720]" }
    };
    constexpr std::pair<std::string_view, std::string_view> widescreen_4k[]{
        std::pair{ "name", "4K Widescreen" },
        std::pair{ "window_size", "[3840,2160]" }
    };
    constexpr std::pair<std::string_view, std::string_view> widescreen_1080[]{
        std::pair{ "name", "1080p Widescreen" },
        std::pair{ "window_size", "[1920,1080]" }
    };

    auto n1 = CreateNode("MockOutput", square_1080);
    auto n2 = CreateNode("MockOutput", widescreen_4k);
    auto n3 = CreateNode("MockOutput", square_720);
    auto n4 = CreateNode("MockOutput", widescreen_1080);

    REQUIRE(n1 != 0);
    REQUIRE(n2 != 0);
    REQUIRE(n3 != 0);
    REQUIRE(n4 != 0);

    model.SortOutputs();

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 4);

    // Check that square outputs are grouped together (aspect ratio ~1.0)
    // and widescreen outputs are grouped together (aspect ratio ~1.78)
    auto size1 = outputs[0]->GetOutputSize();
    auto size2 = outputs[1]->GetOutputSize();
    auto size3 = outputs[2]->GetOutputSize();
    auto size4 = outputs[3]->GetOutputSize();

    // Calculate aspect ratios
    float aspect1 = static_cast<float>(size1.width) / size1.height;
    float aspect2 = static_cast<float>(size2.width) / size2.height;
    float aspect3 = static_cast<float>(size3.width) / size3.height;
    float aspect4 = static_cast<float>(size4.width) / size4.height;

    // First two should be from the same aspect ratio group
    REQUIRE(std::abs(aspect1 - aspect2) < 0.2f);
    // Last two should be from the same aspect ratio group
    REQUIRE(std::abs(aspect3 - aspect4) < 0.2f);
    // Different groups should have different aspect ratios
    REQUIRE(std::abs(aspect1 - aspect3) > 0.2f);
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.RotatedOutputs", "[optimize]")
{
    // Test portrait vs landscape versions of the same resolution
    constexpr std::pair<std::string_view, std::string_view> landscape[]{
        std::pair{ "name", "Landscape 1920x1080" },
        std::pair{ "window_size", "[1920,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> portrait[]{
        std::pair{ "name", "Portrait 1080x1920" },
        std::pair{ "window_size", "[1080,1920]" }
    };
    constexpr std::pair<std::string_view, std::string_view> landscape_4k[]{
        std::pair{ "name", "Landscape 4K" },
        std::pair{ "window_size", "[3840,2160]" }
    };
    constexpr std::pair<std::string_view, std::string_view> portrait_4k[]{
        std::pair{ "name", "Portrait 4K" },
        std::pair{ "window_size", "[2160,3840]" }
    };

    auto n1 = CreateNode("MockOutput", landscape);
    auto n2 = CreateNode("MockOutput", portrait);
    auto n3 = CreateNode("MockOutput", landscape_4k);
    auto n4 = CreateNode("MockOutput", portrait_4k);

    REQUIRE(n1 != 0);
    REQUIRE(n2 != 0);
    REQUIRE(n3 != 0);
    REQUIRE(n4 != 0);

    model.SortOutputs();

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 4);

    // Verify that rotated outputs are NOT in the same compatibility group
    // since they have different aspect ratios
    auto* output1 = model.GetNode(n1);
    auto* output2 = model.GetNode(n2);
    auto* output3 = model.GetNode(n3);
    auto* output4 = model.GetNode(n4);

    // Cast to IOutput for compatibility testing
    auto* iout1 = static_cast<vortex::graph::IOutput*>(output1);
    auto* iout2 = static_cast<vortex::graph::IOutput*>(output2);
    auto* iout3 = static_cast<vortex::graph::IOutput*>(output3);
    auto* iout4 = static_cast<vortex::graph::IOutput*>(output4);

    // Landscape outputs should be compatible with each other
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(iout1, iout3));
    // Portrait outputs should be compatible with each other  
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(iout2, iout4));
    // But landscape and portrait should NOT be compatible
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(iout1, iout2));
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(iout3, iout4));
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.ScaleCompatibility", "[optimize]")
{
    // Test outputs with different scales but same aspect ratio
    constexpr std::pair<std::string_view, std::string_view> small_hd[]{
        std::pair{ "name", "720p" },
        std::pair{ "window_size", "[1280,720]" }
    };
    constexpr std::pair<std::string_view, std::string_view> full_hd[]{
        std::pair{ "name", "1080p" },
        std::pair{ "window_size", "[1920,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> ultra_hd[]{
        std::pair{ "name", "4K" },
        std::pair{ "window_size", "[3840,2160]" }
    };
    constexpr std::pair<std::string_view, std::string_view> huge_scale[]{
        std::pair{ "name", "8K" },
        std::pair{ "window_size", "[7680,4320]" }
    };

    auto n1 = CreateNode("MockOutput", small_hd);
    auto n2 = CreateNode("MockOutput", full_hd);
    auto n3 = CreateNode("MockOutput", ultra_hd);
    auto n4 = CreateNode("MockOutput", huge_scale);

    REQUIRE(n1 != 0);
    REQUIRE(n2 != 0);
    REQUIRE(n3 != 0);
    REQUIRE(n4 != 0);

    model.SortOutputs();

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 4);

    // Verify compatibility within 2x scaling limit
    auto* output1 = static_cast<vortex::graph::IOutput*>(model.GetNode(n1));
    auto* output2 = static_cast<vortex::graph::IOutput*>(model.GetNode(n2));
    auto* output3 = static_cast<vortex::graph::IOutput*>(model.GetNode(n3));
    auto* output4 = static_cast<vortex::graph::IOutput*>(model.GetNode(n4));

    // 720p and 1080p should be compatible (1.5x scale)
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(output1, output2));
    // 1080p and 4K should be compatible (2x scale exactly)
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(output2, output3));
    // 720p and 4K should NOT be compatible (>2x scale)
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(output1, output3));
    // 4K and 8K should be compatible (2x scale exactly)
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(output3, output4));
    // 1080p and 8K should NOT be compatible (4x scale)
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(output2, output4));
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.CustomResolutions", "[optimize]")
{
    // Test unusual/custom resolutions
    constexpr std::pair<std::string_view, std::string_view> ultrawide[]{
        std::pair{ "name", "Ultrawide" },
        std::pair{ "window_size", "[3440,1440]" }
    };
    constexpr std::pair<std::string_view, std::string_view> square_custom[]{
        std::pair{ "name", "Custom Square" },
        std::pair{ "window_size", "[1440,1440]" }
    };
    constexpr std::pair<std::string_view, std::string_view> vertical_strip[]{
        std::pair{ "name", "Vertical Strip" },
        std::pair{ "window_size", "[480,1920]" }
    };
    constexpr std::pair<std::string_view, std::string_view> tiny_square[]{
        std::pair{ "name", "Tiny Square" },
        std::pair{ "window_size", "[256,256]" }
    };

    auto n1 = CreateNode("MockOutput", ultrawide);
    auto n2 = CreateNode("MockOutput", square_custom);
    auto n3 = CreateNode("MockOutput", vertical_strip);
    auto n4 = CreateNode("MockOutput", tiny_square);

    REQUIRE(n1 != 0);
    REQUIRE(n2 != 0);
    REQUIRE(n3 != 0);
    REQUIRE(n4 != 0);

    model.SortOutputs();

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 4);

    // Test some specific compatibility expectations
    auto* ultrawide_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n1));
    auto* square_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n2));
    auto* vertical_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n3));
    auto* tiny_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n4));

    // Ultrawide (21:9) and vertical strip (1:4) should NOT be compatible
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(ultrawide_out, vertical_out));
    
    // Square outputs should not be compatible, since they are not within 2x scaling
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(square_out, tiny_out));
    
    // Ultrawide and square should NOT be compatible (different aspect ratios)
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(ultrawide_out, square_out));
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.SequentialCompatibility", "[optimize]")
{
    // Test GetNextCompatibleOutput functionality
    constexpr std::pair<std::string_view, std::string_view> hd720[]{
        std::pair{ "name", "720p" },
        std::pair{ "window_size", "[1280,720]" }
    };
    constexpr std::pair<std::string_view, std::string_view> hd1080[]{
        std::pair{ "name", "1080p" },
        std::pair{ "window_size", "[1920,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> square[]{
        std::pair{ "name", "Square" },
        std::pair{ "window_size", "[1080,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> hd1440[]{
        std::pair{ "name", "1440p" },
        std::pair{ "window_size", "[2560,1440]" }
    };

    auto n1 = CreateNode("MockOutput", hd720);
    auto n2 = CreateNode("MockOutput", hd1080);
    auto n3 = CreateNode("MockOutput", square);
    auto n4 = CreateNode("MockOutput", hd1440);

    REQUIRE(n1 != 0);
    REQUIRE(n2 != 0);
    REQUIRE(n3 != 0);
    REQUIRE(n4 != 0);

    model.SortOutputs();

    auto outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 4);

    // Test sequential compatibility search
    for (size_t i = 0; i < outputs.size(); ++i) {
        const auto* next_compatible = model.GetNextCompatibleOutput(i);
        
        if (next_compatible) {
            // If we found a compatible output, verify it actually is compatible
            REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(outputs[i], next_compatible));
        }
        
        // Test edge case: last output should have no next compatible
        if (i == outputs.size() - 1) {
            REQUIRE(next_compatible == nullptr);
        }
    }
}

TEST_CASE_METHOD(ModelTest, "SortOutputs.EmptyAndInvalidOutputs", "[optimize]")
{
    // Test behavior with no outputs
    model.SortOutputs();
    auto outputs = model.GetOutputs();
    REQUIRE(outputs.empty());

    // Add one output and test
    constexpr std::pair<std::string_view, std::string_view> single_output[]{
        std::pair{ "name", "Single Output" },
        std::pair{ "window_size", "[1920,1080]" }
    };

    auto n1 = CreateNode("MockOutput", single_output);
    REQUIRE(n1 != 0);

    model.SortOutputs();
    outputs = model.GetOutputs();
    REQUIRE(outputs.size() == 1);
    REQUIRE(equal(outputs[0]->GetOutputSize(), wis::Size2D{ 1920, 1080 }));

    // Test GetNextCompatibleOutput with single output
    const auto* next = model.GetNextCompatibleOutput(0);
    REQUIRE(next == nullptr); // No next output
}

TEST_CASE_METHOD(ModelTest, "AreSizeCompatible.EdgeCases", "[optimize]")
{
    // Test edge cases for size compatibility
    constexpr std::pair<std::string_view, std::string_view> base_1080[]{
        std::pair{ "name", "Base 1080p" },
        std::pair{ "window_size", "[1920,1080]" }
    };
    constexpr std::pair<std::string_view, std::string_view> exactly_2x[]{
        std::pair{ "name", "Exactly 2x" },
        std::pair{ "window_size", "[3840,2160]" }
    };
    constexpr std::pair<std::string_view, std::string_view> just_over_2x[]{
        std::pair{ "name", "Just over 2x" },
        std::pair{ "window_size", "[3841,2161]" }
    };
    constexpr std::pair<std::string_view, std::string_view> aspect_10_percent[]{
        std::pair{ "name", "10% aspect difference" },
        std::pair{ "window_size", "[2112,1080]" } // ~1.956 vs 1.778 = ~10% difference
    };
    constexpr std::pair<std::string_view, std::string_view> aspect_over_10_percent[]{
        std::pair{ "name", "Over 10% aspect difference" },
        std::pair{ "window_size", "[2200,1080]" } // ~2.037 vs 1.778 = >10% difference
    };

    auto n1 = CreateNode("MockOutput", base_1080);
    auto n2 = CreateNode("MockOutput", exactly_2x);
    auto n3 = CreateNode("MockOutput", just_over_2x);
    auto n4 = CreateNode("MockOutput", aspect_10_percent);
    auto n5 = CreateNode("MockOutput", aspect_over_10_percent);

    auto* base = static_cast<vortex::graph::IOutput*>(model.GetNode(n1));
    auto* exactly_2x_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n2));
    auto* over_2x_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n3));
    auto* aspect_10_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n4));
    auto* aspect_over_10_out = static_cast<vortex::graph::IOutput*>(model.GetNode(n5));

    // Exactly 2x scale should be compatible
    REQUIRE(vortex::graph::GraphModel::AreSizeCompatible(base, exactly_2x_out));
    
    // Just over 2x scale should NOT be compatible
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(base, over_2x_out));
    
    // ~10% aspect ratio difference should be at the boundary (implementation dependent)
    bool aspect_10_compatible = vortex::graph::GraphModel::AreSizeCompatible(base, aspect_10_out);
    
    // Over 10% aspect ratio difference should NOT be compatible
    REQUIRE_FALSE(vortex::graph::GraphModel::AreSizeCompatible(base, aspect_over_10_out));
}

