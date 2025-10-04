#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>

namespace vortex::graph {
// Blend node is a filter that blends colors with a specified factor or a mask.
class Select : public vortex::graph::FilterImpl<Select, SelectProperties, 2, 1>
{
public:
    Select(const vortex::Graphics& gfx, SerializedProperties props);

public:
    // Override the Evaluate method to perform blending
    virtual bool Evaluate(const vortex::Graphics& gfx,
        RenderProbe& probe,
        const RenderPassForwardDesc* output_info = nullptr) override
    {
        uint32_t index = static_cast<uint32_t>(std::clamp(input_index, 0, 1));
        if (auto& sink = GetSinks()[index]; sink)
        {
            return sink.source_node->Evaluate(gfx, probe, output_info);
        }
        return false;
    }
};
}