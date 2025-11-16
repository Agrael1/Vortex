#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <wisdom/wisdom.hpp>

namespace vortex {
struct TransformLazy {
public:
    TransformLazy(const vortex::Graphics& gfx);

public:
    wis::RootSignatureView GetRootSignature() const noexcept { return _root_signature; }
    wis::PipelineView GetPipelineState() const noexcept { return _pipeline_state; }
    wis::SamplerView GetSampler() const noexcept { return _sampler; }

private:
    wis::RootSignature _root_signature; // Root signature for the transform node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    wis::Sampler _sampler; // Sampler for the texture
};

// Transform node is a filter that applies 2D transformations (translate, rotate, scale) to an image
class Transform : public vortex::graph::FilterImpl<Transform, TransformProperties, 1, 1>
{
public:
    Transform(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props)
        , _lazy_data(gfx)
    {
    }

public:
    // Override the Evaluate method to perform transformation
    virtual bool Evaluate(const vortex::Graphics& gfx,
                          RenderProbe& probe,
                          const RenderPassForwardDesc* output_info = nullptr) override;

private:
    [[no_unique_address]] lazy_ptr<TransformLazy> _lazy_data; // Lazy data for static resources
};
} // namespace vortex