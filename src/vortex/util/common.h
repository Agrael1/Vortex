#pragma once
#include <unordered_map>
#include <string_view>
#include <wisdom/generated/api/api.hpp>
#include <vortex/util/unique_any.h>
#include <format>
#include <vortex/util/lib/reflect.h>

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
struct std::formatter<wis::DataFormat> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs DataFormat enum name
    template<typename FormatContext>
    auto format(wis::DataFormat format, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", reflect::enum_name(format));
    }
};

template<>
struct std::formatter<wis::MemoryType> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs MemoryType enum name
    template<typename FormatContext>
    auto format(wis::MemoryType memory_type, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", reflect::enum_name(memory_type));
    }
};

template<>
struct std::formatter<wis::TextureUsage> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs TextureUsage flags
    template<typename FormatContext>
    auto format(wis::TextureUsage usage, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", reflect::flag_names(usage));
    }
};

template<>
struct std::formatter<wis::TextureLayout> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs TextureLayout enum name
    template<typename FormatContext>
    auto format(wis::TextureLayout layout, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", reflect::enum_name(layout));
    }
};

template<>
struct std::formatter<wis::SampleRate> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs SampleCount enum name
    template<typename FormatContext>
    auto format(wis::SampleRate sample_count, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", reflect::enum_name(sample_count));
    }
};

template<>
struct std::formatter<wis::Size3D> {
    bool use_x_format = false;
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();

        // Parse format specifiers until we hit '}'
        while (it != ctx.end() && *it != '}') {
            if (*it == 'x') {
                use_x_format = true;
            }
            ++it;
        }

        return it; // Should point to '}' or end
    }
    // Format function that outputs Size2D fields
    template<typename FormatContext>
    auto format(wis::Size3D size, FormatContext& ctx) const
    {
        // If the format is {x} we do {}x{}x{} for Size3D
        if (use_x_format) {
            return std::format_to(ctx.out(), "{}x{}x{}", size.width, size.height, size.depth_or_layers);
        }
        return std::format_to(ctx.out(), "Size(width={}, height={}, depth_or_layers={})",
                              size.width, size.height, size.depth_or_layers);
    }
};

template<>
struct std::formatter<wis::Size2D> {
    bool use_x_format = false;
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();

        // Parse format specifiers until we hit '}'
        while (it != ctx.end() && *it != '}') {
            if (*it == 'x') {
                use_x_format = true;
            }
            ++it;
        }

        return it; // Should point to '}' or end
    }
    // Format function that outputs Size2D fields
    template<typename FormatContext>
    auto format(wis::Size2D size, FormatContext& ctx) const
    {
        // If the format is {x} we do {}x{} for Size2D
        if (use_x_format) {
            return std::format_to(ctx.out(), "{}x{}", size.width, size.height);
        }
        return std::format_to(ctx.out(), "Size(width={}, height={})", size.width, size.height);
    }
};

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
                              "TextureDesc{{format={},"
                              "size = {:x},"
                              //"{}"
                              //", layout={}{}"
                              //", usage={})",
                              "}}",
                              desc.format,
                              desc.size
                              //(desc.mip_levels > 1 ? std::format(", mip_levels={}", desc.mip_levels) : ""),
                              // desc.layout,
                              //(int(desc.sample_count) > 1 ? std::format(" , sample_count={}", reflect::enum_name(desc.sample_count)) : ""),
                              // desc.usage
        );
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

using unique_file = unique_any<std::FILE*, std::fclose>;

/**
 * Based on boost::hash_combine. Since Boost is licensed under the Boost
 * Software License, we include a copy of the license here.
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
template<class T>
inline std::size_t hash_combine_one(std::size_t& seed, const T& v) noexcept(std::is_nothrow_invocable_v<std::hash<T>, T>)
{
    return seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename... Args>
inline std::size_t hash_combine(Args&&... args) noexcept
{
    std::size_t seed = 0;
    (hash_combine_one(seed, std::forward<Args>(args)), ...);
    return seed;
}
} // namespace vortex
