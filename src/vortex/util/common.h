#pragma once
#include <unordered_map>
#include <string_view>
#include <wisdom/generated/api/api.hpp>
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

template<typename T, auto Deleter, bool pp = false>
struct unique_any {
    static constexpr bool is_pointer = std::is_pointer_v<T>;
    using ptr_type = std::add_pointer_t<std::remove_pointer_t<T>>;
    using cptr_type = const ptr_type;
    using ref_type = std::conditional_t<is_pointer, T, T&>;
    using cref_type = std::conditional_t<is_pointer, T, const T&>;



    T data;

    ~unique_any()
    {
        clear();
    }

    unique_any() = default;
    unique_any(T ptr)
        : data(ptr) { }

    // Non-copyable.
    unique_any(const unique_any&) = delete;
    unique_any& operator=(const unique_any&) = delete;

    // Movable.
    unique_any(unique_any&& other) noexcept
        : data(other.data)
    {
        other.data = nullptr;
    }
    unique_any& operator=(unique_any&& other) noexcept
    {
        if (this != std::addressof(other)) {
            if (data) {
                clear();
            }
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }

public:
    explicit operator bool() const noexcept
    {
        return data != nullptr;
    }
    decltype(auto) operator&() noexcept
    {
        return &data;
    }
    decltype(auto) operator&() const noexcept
    {
        return &data;
    }
    decltype(auto) operator->() noexcept
    {
        if constexpr (is_pointer) {
            return data;
        } else {
            return &data;
        }
    }
    cref_type get() const noexcept
    {
        return data;
    }
    T release() noexcept
    {
        T temp = data;
        data = nullptr;
        return temp;
    }
    void reset(cref_type ptr) noexcept
    {
        if (data) {
            clear();
        }
        data = ptr;
    }
    void clear() noexcept
    {
        if constexpr (pp) {
            // If Deleter is a pointer to a function, we call it directly.
            Deleter(&data);
        } else {
            // Otherwise, we assume Deleter is a callable object.
            Deleter(data);
        }
    }
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
inline void hash_combine(std::size_t& seed, const T& v) noexcept(std::is_nothrow_invocable_v<std::hash<T>, T>)
{
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace vortex

