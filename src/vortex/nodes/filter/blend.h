#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <wisdom/wisdom.hpp>
#include <array>

namespace vortex {
class BlendLazy
{
public:
    static constexpr uint32_t hw_blend_mode_count = 7; // Number of blend modes supported
public:
    BlendLazy(const vortex::Graphics& gfx);

public:
    wis::RootSignatureView GetRootSignature() const noexcept { return _root_signature; }
    wis::PipelineView GetPipelineState(BlendMode mode) const noexcept
    {
        uint32_t index = static_cast<uint32_t>(mode);
        if (index >= hw_blend_mode_count) {
            index = 0; // Default to Normal if out of range
        }
        return _pipeline_states[index];
    }
    wis::SamplerView GetSampler() const noexcept { return _sampler; }

private:
    std::array<wis::PipelineState, hw_blend_mode_count> _pipeline_states = {};
    wis::RootSignature _root_signature;
    wis::Sampler _sampler;
};

// Blend node is a filter that blends colors with a specified factor or a mask.
class Blend : public vortex::graph::FilterImpl<Blend, BlendProperties, 2, 1>
{
public:
    Blend(const vortex::Graphics& gfx, SerializedProperties props);

public:
    // Override the Evaluate method to perform blending
    virtual bool Evaluate(const vortex::Graphics& gfx,
                          RenderProbe& probe,
                          const RenderPassForwardDesc* output_info = nullptr) override;

private:
    [[no_unique_address]] lazy_ptr<BlendLazy> _lazy_data; // Lazy data for static resources
};
} // namespace vortex