#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <vortex/graph/interfaces.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/gfx/texture_pool.h>
#include <vortex/probe.h>
#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <ranges>

namespace vortex {
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::graph::OutputImpl<WindowOutput, WindowOutputProperties>
{
    static constexpr wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for
                                                                           // render targets
    static constexpr size_t max_swapchain_images = 2; // Maximum number of swapchain images
public:
    WindowOutput(const vortex::Graphics& gfx, SerializedProperties props);

public:
    void Throttle() noexcept;
    void Present(const vortex::Graphics& gfx) noexcept;

    // Setters for properties
    void SetName(std::string_view name, bool notify = false)
    {
        WindowOutputProperties::SetName(name, true);
        if (IsInitialized()) {
            _window.SetTitle(name.data());
        }
    }
    void SetWindowSize(DirectX::XMUINT2 size, bool notify = false)
    {
        WindowOutputProperties::SetWindowSize(size, true);
        if (!IsInitialized()) {
            return; // Do not resize the window if it is not initialized
        }
        _window.SetSize(int(size.x), int(size.y));
        _resized = true; // Mark as resized to trigger swapchain resize
    }

public:
    virtual vortex::ratio32_t GetOutputFPS() const noexcept { return GetFramerate(); }
    virtual wis::Size2D GetOutputSize() const noexcept { return { window_size.x, window_size.y }; }
    virtual void Update(const vortex::Graphics& gfx) override;
    virtual bool Evaluate(const vortex::Graphics& gfx) override;

private:
    vortex::ui::SDLWindow _window;

public:
    wis::SwapChain _swapchain;
    wis::CommandList _command_lists[vortex::max_frames_in_flight]; ///< Command list for rendering
    wis::RenderTarget _render_targets[max_swapchain_images]; ///< Render target for the swapchain
    std::span<const wis::Texture> _textures; ///< Textures for the swapchain

    uint32_t _frame_index = 0; ///< Current frame index for double buffering

    wis::Fence _fence; ///< Fence for synchronization
    uint64_t _fence_value = 1; ///< Current fence value for synchronization
    uint64_t _fence_values[vortex::max_frames_in_flight] = { 1, 0 }; ///< Current fence value for
                                                                     ///< synchronization
    bool _resized = false; ///< Flag to indicate if the window has been resized

    vortex::DescriptorBuffer _desc_buffer; ///< Descriptor buffer for the output
    vortex::TexturePool _texture_pool; ///< Texture pool for the output
};
} // namespace vortex