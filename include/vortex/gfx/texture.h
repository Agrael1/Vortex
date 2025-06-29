#pragma once
#include <wisdom/wisdom.hpp>
#include <util/common.h>

namespace vortex {
class Graphics;
class Texture2D
{
public:
    Texture2D() = default;
    explicit Texture2D(wis::Texture texture, wis::Size2D size = {}, wis::DataFormat format = wis::DataFormat::Unknown)
        : _texture(std::move(texture))
    {
    }
    operator bool() const noexcept
    {
        return static_cast<bool>(_texture);
    }

public:
    template<typename Self>
    decltype(auto) Get(this Self&& self) noexcept
    {
        return self._texture;
    }
    wis::Size2D GetSize() const noexcept
    {
        return _size;
    }
    wis::DataFormat GetFormat() const noexcept
    {
        return _format;
    }

    wis::RenderTarget CreateRenderTarget(vortex::Graphics& gfx) const noexcept;

private:
    wis::Texture _texture;
    wis::Size2D _size;
    wis::DataFormat _format;
};
} // namespace vortex

namespace std {
// spec for format
template<>
struct std::formatter<vortex::Texture2D> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    // Format function that outputs TextureDesc fields
    template<typename FormatContext>
    auto format(const vortex::Texture2D& texture, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "Texture2D(size={}, format={})",
                              texture.GetSize(),
                              reflect::enum_name(texture.GetFormat()));
    }
};

} // namespace std