#pragma once
#include <unordered_map>
#include <string_view>
#include <wisdom/generated/api/api.hpp>
#include <format>
#include <util/reflect.h>
#include <bit>

namespace reflect {
template<typename T>
    requires wis::is_flag_enum<T>::value
std::string flag_names(T flags) noexcept
{
    using UT = std::underlying_type_t<T>;
    UT mask = static_cast<UT>(flags);
    std::string result;
    bool first = true;
    // iterate bit by bit; when bit shifts to zero we have covered all bits.
    for (UT bit = 1; bit != 0; bit <<= 1) {
        if (mask & bit) {
            if (!first) {
                result.append(" | ");
            }
            result.append(reflect::enum_name(static_cast<T>(bit)));
            first = false;
        }
    }
    if (result.empty()) {
        result = reflect::enum_name(static_cast<T>(0));
    }
    return result;
}
} // namespace reflect

template<>
struct std::formatter<wis::TextureDesc> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    // Format function that outputs TextureDesc fields
    template<typename FormatContext>
    auto format(const wis::TextureDesc& desc, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "TextureDesc(format={}, Size(width={}, height={}{})"
                              "{}"
                              ", layout={}{}"
                              ", usage={})",
                              reflect::enum_name(desc.format),
                              desc.size.width,
                              desc.size.height,
                              (desc.size.depth_or_layers > 1 ? std::format(", depth={}", desc.size.depth_or_layers) : ""),
                              (desc.mip_levels > 1 ? std::format(", mip_levels={}", desc.mip_levels) : ""),
                              reflect::enum_name(desc.layout),
                              (int(desc.sample_count) > 1 ? std::format(" , sample_count={}", reflect::enum_name(desc.sample_count)) : ""),
                              reflect::flag_names(desc.usage));
    }
};
template<>
struct std::formatter<wis::TextureRegion> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    // Format function that outputs TextureDesc fields
    template<typename FormatContext>
    auto format(const wis::TextureRegion& desc, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "Offset(width={}, height={}{}), Size(width={}, height={}{})"
                              "{}"
                              "{}"
                              ", format={})",
                              desc.offset.width,
                              desc.offset.height,
                              (desc.offset.depth_or_layers > 0 ? std::format(", depth={}", desc.offset.depth_or_layers) : ""),

                              desc.size.width,
                              desc.size.height,
                              (desc.size.depth_or_layers > 1 ? std::format(", depth={}", desc.size.depth_or_layers) : ""),

                              desc.mip > 1 ? std::format(", mip={}", desc.mip) : "",
                              desc.array_layer > 1 ? std::format(", array_layer={}", desc.array_layer) : "",
                              reflect::enum_name(desc.format));
    }
};

namespace vortex {
inline bool success(wis::Result result) noexcept
{
    return int(result.status) >= 0;
}

struct string_hash : public std::hash<std::string_view> {
    using is_transparent = void;
};
struct string_equal : public std::equal_to<std::string_view> {
    using is_transparent = void;
};

} // namespace vortex