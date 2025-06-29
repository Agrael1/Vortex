#pragma once
#include <utility>
#include <wisdom/wisdom.hpp>

namespace vortex {
class Graphics;
class Swapchain
{
public:
    Swapchain() = default;
    Swapchain(const vortex::Graphics& gfx, wis::SwapChain&& swapchain, const wis::SwapchainDesc& desc)
        : swap(std::move(swapchain))
        , desc(desc)
    {
    }
    //    ~Swapchain()
    //    {
    //        if (swap) {
    //            Throttle();
    //        }
    //    }
    //
    // public:
    //    void Throttle() noexcept
    //    {
    //        CheckResult(fence.Wait(fence_values[frame_index] - 1));
    //    }
    //    bool Present(const wis::CommandQueue& main_queue);
    //    bool Present(w::Graphics& gfx);
    //    void Resize(const wis::Device& device, uint32_t width, uint32_t height);
    //    void Resize(w::Graphics& gfx, uint32_t width, uint32_t height);
    //    uint32_t CurrentFrame() const
    //    {
    //        return swap.GetCurrentIndex();
    //    }
    //    const wis::SwapChain& GetSwapChain() const
    //    {
    //        return swap;
    //    }
    //    uint32_t GetWidth() const
    //    {
    //        return width;
    //    }
    //    uint32_t GetHeight() const
    //    {
    //        return height;
    //    }
    //
    //    std::span<const wis::Texture> GetTextures() const
    //    {
    //        return textures;
    //    }
    //    const wis::Texture& GetTexture(size_t i) const
    //    {
    //        return textures[i];
    //    }
    //    const wis::RenderTarget& GetRenderTarget(size_t i) const
    //    {
    //        return render_targets[i];
    //    }
    //
private:
    wis::SwapChain swap;
    //wis::Fence fence;
    //uint64_t fence_value = 1;
    //uint64_t frame_index = 0;
    //std::array<uint64_t, flight_frames> fence_values{ 1, 0 };
    //
    //std::span<const wis::Texture> textures;
    //std::array<wis::RenderTarget, swap_frames> render_targets;

    wis::SwapchainDesc desc;
};

} // namespace vortex