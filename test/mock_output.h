#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/probe.h>

namespace vortex {
class MockOutput : public vortex::graph::OutputImpl<MockOutput, WindowOutputProperties>
{
    static constexpr wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for render targets
public:
    MockOutput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props)
    {
        
    }

public:
    void SetWindowSize(DirectX::XMUINT2 value, bool notify = false)
    {
        WindowOutputProperties::SetWindowSize(value, notify);
        _resized = IsInitialized();
    }

public:
    virtual bool IsReady() const noexcept
    {
        // Check if the swapchain is ready for rendering
        return true;
    }
    virtual vortex::ratio32_t GetOutputFPS() const noexcept
    {
        return framerate;
    }
    virtual wis::Size2D GetOutputSize() const noexcept
    {
        return { window_size.x, window_size.y };
    }

    bool Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override
    {
        return true; // Mock output always succeeds
    }

private:
    bool _resized = false; ///< Flag to indicate if the output has been resized
};
} // namespace vortex