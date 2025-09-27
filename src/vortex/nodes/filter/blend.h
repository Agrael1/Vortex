#pragma once

#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>

namespace vortex {
// Blend node is a filter that blends colors with a specified factor or a mask.
class Blend : public vortex::graph::FilterImpl<Blend, BlendProperties, 2, 1>
{
public:
    Blend(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props)
    {
    }

    // Override the Evaluate method to perform blending
    virtual bool Evaluate(const vortex::Graphics& gfx, RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override
    {
        return false; // Placeholder implementation
    }
};
} // namespace vortex