#pragma once
#include <vortex/util/common.h>

namespace vortex {
struct RenderTargetRequest {
    wis::Size2D size;
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // For now assume RGBA8

    // Comparison operators for hash map usage
    bool operator==(const RenderTargetRequest& other) const noexcept
    {
        return std::memcmp(this, &other, sizeof(RenderTargetRequest)) == 0;
    }
};

class TexturePool
{

};
} // namespace vortex

// #include <vortex/gfx/texture.h>
// #include <vortex/util/common.h>
// #include <wisdom/wisdom.hpp>
// #include <unordered_map>
// #include <vector>
// #include <memory>
//
// namespace vortex {
// class Graphics;
//
//// Render Target will always be 2D textures, so we can simplify the request
// struct RenderTargetRequest {
//     wis::Size2D size;
//     wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // For now assume RGBA8
//
//     // Comparison operators for hash map usage
//     bool operator==(const RenderTargetRequest& other) const noexcept
//     {
//         return std::memcmp(this, &other, sizeof(RenderTargetRequest)) == 0;
//     }
// };
//
//
////// Texture storage manages a pool of textures for efficient allocation
////class TextureStorage
////{
////public:
////    TextureStorage() = default;
////    explicit TextureStorage(const vortex::Graphics& gfx);
////
////    // Request a texture with the given specifications
////    // Returns a handle that manages the texture lifetime
////    TextureHandle RequestTexture(const TextureRequest& request);
////
////    // Get statistics about texture usage
////    struct Statistics {
////        size_t total_textures_created = 0;
////        size_t active_allocations = 0;
////        size_t available_in_pool = 0;
////        size_t memory_usage_bytes = 0;
////    };
////    Statistics GetStatistics() const;
////
////    // Clean up unused textures (call periodically to free memory)
////    void Cleanup(uint32_t frames_unused_threshold = 60);
////
////    // Clear all textures (useful for shutdown)
////    void Clear();
////
////    // Called each frame to update frame counter for cleanup
////    void OnFrameEnd() { ++_current_frame; }
////
////private:
////    friend class TextureHandle;
////
////    // Internal texture entry
////    struct TextureEntry {
////        vortex::Texture2D texture;
////        uint32_t last_used_frame = 0;
////        bool in_use = false;
////    };
////
////    // Return a texture to the pool
////    void ReturnTexture(const TextureRequest& request, vortex::Texture2D texture);
////
////    // Create a new texture
////    vortex::Texture2D CreateTexture(const TextureRequest& request);
////
////    const vortex::Graphics* _graphics = nullptr;
////
////    // Pool of textures organized by request type
////    std::unordered_map<TextureRequest, std::vector<std::unique_ptr<TextureEntry>>> _texture_pools;
////
////    // Statistics
////    mutable Statistics _stats;
////    uint32_t _current_frame = 0;
////};
////
////} // namespace vortex
//
namespace std {
// Formatter for TextureRequest (requires reflection header to be included where used)
template<>
struct std::formatter<vortex::RenderTargetRequest> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const vortex::RenderTargetRequest& req, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "TextureRequest({:x}, format={})",
                              req.size,
                              req.format);
    }
};
// Hash specialization for TextureRequest
template<>
struct hash<vortex::RenderTargetRequest> {
    std::size_t operator()(const vortex::RenderTargetRequest& req) const noexcept
    {
        // Simple hash combination
        return vortex::hash_combine(req.size.width, req.size.height, static_cast<uint32_t>(req.format));
    }
};
} // namespace std