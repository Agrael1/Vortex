#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/ndi/ndi_swapchain.h>
#include <vortex/audio/audio_buffer.h>

namespace vortex {
struct RenderProbe;
struct RenderPassForwardDesc;

class NDIOutput : public vortex::graph::OutputImpl<NDIOutput, NDIOutputProperties, 2>
{
    static constexpr std::size_t audio_sample_rate = 48000; // Fixed sample rate for NDI audio
    static constexpr std::size_t max_audio_channels = 2; // NDI supports up to 8 channels, but we'll limit to stereo for simplicity
    static constexpr wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for render targets
public:
    NDIOutput(const vortex::Graphics& gfx, SerializedProperties props);
    ~NDIOutput()
    {
        Throttle(); // Wait for GPU to finish before destroying resources
    }

public:
    // Properties
    void SetName(std::string_view value, bool notify = false)
    {
        NDIOutputProperties::SetName(value, notify);
        if (IsInitialized()) {
            _swapchain.SetName(value);
        }
    }
    void SetWindowSize(DirectX::XMUINT2 value, bool notify = false)
    {
        NDIOutputProperties::SetWindowSize(value, notify);
        _resized = IsInitialized();
    }
    void SetFramerate(vortex::ratio32_t value, bool notify = false)
    {
        NDIOutputProperties::SetFramerate(value, notify);
        if (IsInitialized()) {
            _swapchain.SetFramerate(value);
        }
    }

public:
    virtual vortex::ratio32_t GetOutputFPS() const noexcept
    {
        return GetFramerate();
    }
    virtual wis::Size2D GetOutputSize() const noexcept
    {
        return { window_size.x, window_size.y };
    }

    virtual void Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe) override;
    virtual bool Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override;

private:
    void Throttle() const;
    bool EvaluateAudio();
    bool EvaluateVideo(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr);
    uint64_t CurrentFrameIndex() const noexcept
    {
        return (_fence_value - 1) % vortex::max_frames_in_flight;
    }

private:
    NDISwapchain _swapchain;
    wis::CommandList _command_lists[vortex::max_frames_in_flight];
    wis::RenderTarget _render_targets[NDISwapchain::max_swapchain_images];

    wis::Fence _fence; ///< Fence for synchronization
    uint64_t _fence_value = 1; ///< Current fence value for synchronization

    vortex::AudioBuffer _audio_buffer; ///< Audio buffer for storing audio samples (planar float)
    uint64_t _last_audio_pts = 0; ///< Last audio PTS sent to NDI

    std::vector<float> _audio_samples;
    bool _resized = false; ///< Flag to indicate if the output has been resized
};
} // namespace vortex