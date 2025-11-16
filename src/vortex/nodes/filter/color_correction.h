#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <vortex/util/lut_loader.h>
#include <wisdom/wisdom.hpp>
#include <filesystem>

namespace vortex {
struct ColorCorrectionLazy {
public:
    ColorCorrectionLazy(const vortex::Graphics& gfx);

public:
    wis::RootSignatureView GetRootSignature() const noexcept { return _root_signature; }
    wis::PipelineView GetPipelineState() const noexcept { return _pipeline_state; }
    wis::SamplerView GetSampler(LUTInterp interp) const noexcept
    {
        return (interp == LUTInterp::Trilinear) ? _sampler_linear : _sampler_point;
    }

private:
    wis::RootSignature _root_signature;
    wis::PipelineState _pipeline_state;
    wis::Sampler _sampler_linear; // Linear sampling for texture
    wis::Sampler _sampler_point; // Point sampling for 1D LUT
};

// Color correction node applies LUT-based color grading
class ColorCorrection : public vortex::graph::FilterImpl<ColorCorrection,
                                                         ColorCorrectionProperties,
                                                         1,
                                                         1,
                                                         vortex::graph::EvaluationStrategy::Static>
{
public:
    ColorCorrection(const vortex::Graphics& gfx, SerializedProperties props);

public:
    // Override the Evaluate method
    virtual void Update(const vortex::Graphics& gfx) override;
    virtual bool Evaluate(const vortex::Graphics& gfx,
                          RenderProbe& probe,
                          const RenderPassForwardDesc* output_info = nullptr) override;

    // Property change handlers
    void SetLut(std::string_view path, bool notify = true);

private:
    void LoadLUT(const vortex::Graphics& gfx, const std::filesystem::path& path);
    void CreateLUTTexture(const vortex::Graphics& gfx, const LutData& lut_data);

private:
    [[no_unique_address]] lazy_ptr<ColorCorrectionLazy> _lazy_data;

    wis::Texture _lut_texture;
    wis::ShaderResource _lut_srv;
    LutType _lut_type = LutType::Undefined;
    bool _lut_changed = false;
};
} // namespace vortex
