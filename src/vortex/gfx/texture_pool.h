#pragma once
#include <vortex/util/log_storage.h>
#include <vortex/util/common.h>
#include <vortex/consts.h>
#include <wisdom/wisdom.hpp>
#include <unordered_map>
#include <vector>

namespace vortex {
class Graphics;
struct OutputTextureDesc {
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Format of the texture (e.g.,
                                                          // RGBA8Unorm)
    wis::Size2D size; // 99% is a 2D texture

    bool operator==(const OutputTextureDesc& other) const noexcept
    {
        return format == other.format && size.width == other.size.width &&
                size.height == other.size.height;
    }
};

class TexturePool
{
    static constexpr size_t initial_texture_count = 2; // Initial number of textures to allocate
private:
    void ReleaseTexture(size_t index) noexcept
    {
        if (index < _current_ptr->size()) {
            _current_ptr->at(index).in_use = false;
        }
    }
    wis::TextureView GetTextureView(size_t index) noexcept
    {
        if (index < _current_ptr->size()) {
            return (*_current_ptr)[index].texture;
        }
        return wis::TextureView{};
    }
    wis::RenderTargetView GetRTV(size_t index) noexcept
    {
        if (index < _current_ptr->size()) {
            return (*_current_ptr)[index].rtv;
        }
        return wis::RenderTargetView{};
    }
    wis::ShaderResourceView GetSRV(size_t index) noexcept
    {
        if (index < _current_ptr->size()) {
            return (*_current_ptr)[index].srv;
        }
        return wis::ShaderResourceView{};
    }

public:
    struct UseTexture {
        wis::Texture texture; // The texture to use
        wis::RenderTarget rtv; // Render target view for the texture
        wis::ShaderResource srv; // Shader resource view for the texture
        bool in_use = false; // Whether the texture is currently in use
    };
    struct UseTextureView {
        friend class TexturePool;

    private:
        UseTextureView(size_t index, TexturePool& parent) noexcept
            : texture_index(index)
            , parent_entry(parent)
        {
        }

    public:
        ~UseTextureView() noexcept { parent_entry.ReleaseTexture(texture_index); }
        operator bool() const noexcept { return texture_index != npos; }

    public:
        wis::TextureView GetTexture() const noexcept
        {
            return parent_entry.GetTextureView(texture_index);
        }
        wis::ShaderResourceView GetSRV() const noexcept
        {
            return parent_entry.GetSRV(texture_index);
        }
        wis::RenderTargetView GetRTV() const noexcept { return parent_entry.GetRTV(texture_index); }

    private:
        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();
        std::size_t texture_index = npos; // Index of the texture in the pool
        TexturePool& parent_entry; // Reference to the parent entry
    };

public:
    TexturePool() = default;
    TexturePool(const vortex::Graphics& gfx, const OutputTextureDesc& desc) noexcept
        : _desc(desc)
    {
        AllocateTextures(gfx, initial_texture_count);
    }

public:
    void SwapFrame() noexcept
    {
        _current_ptr = &_textures[(_current_ptr - _textures + 1) % max_frames_in_flight];
    }
    bool AllocateTextures(const vortex::Graphics& gfx, size_t count) noexcept;
    const OutputTextureDesc& GetDesc() const noexcept { return _desc; }
    bool ReallocateIfNeeded(const vortex::Graphics& gfx,
                            const OutputTextureDesc& new_desc) noexcept;

    UseTextureView AcquireTexture(const vortex::Graphics& gfx) noexcept
    {
        auto& textures = *_current_ptr;
        for (size_t i = 0; i < textures.size(); ++i) {
            if (!textures[i].in_use) {
                textures[i].in_use = true;
                return UseTextureView(i, *this);
            }
        }
        // Allocate a new texture if none are available
        bool succ = AllocateTextures(gfx, 1);
        if (succ && !textures.empty()) {
            textures.back().in_use = true;
            return UseTextureView(textures.size() - 1, *this);
        }
        return UseTextureView(UseTextureView::npos, *this);
    }

private:
    OutputTextureDesc _desc;
    std::vector<UseTexture> _textures[vortex::max_frames_in_flight];
    std::vector<UseTexture>* _current_ptr = _textures; // Pointer to the current frame's texture
                                                       // list
    vortex::LogView _log = vortex::LogStorage::GetLog(vortex::graphics_log_name);
};
} // namespace vortex