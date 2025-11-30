#pragma once
#include <vortex/util/log_storage.h>
#include <vortex/util/common.h>
#include <vortex/consts.h>
#include <wisdom/wisdom.hpp>
#include <unordered_map>
#include <vector>

namespace vortex {
class Graphics;
class TexturePool;

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

struct UseTexture {
    wis::Texture texture; // The texture to use
    wis::RenderTarget rtv; // Render target view for the texture
    wis::ShaderResource srv; // Shader resource view for the texture
    std::uint32_t generation = invalid_generation; // Generation of the texture (for tracking usage)
    bool block = false;
};

struct UseTextureView {
    friend class TexturePool;

private:
    UseTextureView(std::uint32_t index, std::uint32_t previous_gen, TexturePool& parent) noexcept
        : texture_index(index)
        , parent_entry(parent)
        , previous_gen(previous_gen)
    {
    }

public:
    ~UseTextureView() noexcept;
    operator bool() const noexcept { return texture_index != npos; }

public:
    wis::TextureView GetTexture() const noexcept;
    wis::ShaderResourceView GetSRV() const noexcept;
    wis::RenderTargetView GetRTV() const noexcept;
    bool RequiresBarrier() const noexcept { return previous_gen != invalid_generation; }
    uint32_t GetIndex() const noexcept { return texture_index; }

private:
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();
    std::uint32_t texture_index = npos; // Index of the texture in the pool
    std::uint32_t previous_gen = invalid_generation; // Generation of the texture
    TexturePool& parent_entry; // Reference to the parent entry
};

class TexturePool
{
    friend class UseTextureView;
    static constexpr size_t initial_texture_count = 2; // Initial number of textures to allocate
private:
    void ReleaseTexture(size_t index, std::uint32_t previous_gen) noexcept
    {
        if (index < _current_ptr->size()) {
            _current_ptr->at(index).generation = previous_gen; // Restore previous generation
            _current_ptr->at(index).block = false; // Unblock the texture
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

    // Acquires a texture from the pool for use
    UseTextureView AcquireTexture(const vortex::Graphics& gfx,
                                  uint32_t current_gen,
                                  uint32_t rt_gen) noexcept
    {
        auto& textures = *_current_ptr;
        for (size_t i = 0; i < textures.size(); ++i) {
            auto gen = textures[i].generation;
            if (gen == invalid_generation || (!textures[i].block) && gen != rt_gen && gen != current_gen) {
                textures[i].generation = current_gen;
                return UseTextureView(i, gen, *this);
            }
        }

        // Allocate a new texture if none are available
        bool succ = AllocateTextures(gfx, _textures[0].size() + 1);
        vortex::info("TexturePool: Allocated new texture, total count: {}", _textures[0].size());
        if (succ && !textures.empty()) {
            textures.back().generation = current_gen;
            return UseTextureView(textures.size() - 1, invalid_generation, *this);
        }
        return UseTextureView(UseTextureView::npos, invalid_generation, *this);
    }

    void BlockTexture(size_t index) noexcept
    {
        if (index < _current_ptr->size()) {
            _current_ptr->at(index).block = true;
        }
    }

private:
    OutputTextureDesc _desc;
    std::vector<UseTexture> _textures[vortex::max_frames_in_flight];
    std::vector<UseTexture>* _current_ptr = _textures; // Pointer to the current frame's texture
                                                       // list
    vortex::LogView _log = vortex::LogStorage::GetLog(vortex::graphics_log_name);
};
} // namespace vortex