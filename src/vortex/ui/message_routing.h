#pragma once
#if __has_include(<include/cef_string.h>)
#include <include/cef_string.h>
#else
#include <include/internal/cef_string.h>
#endif
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>

namespace vortex::ui {
struct RoutedMessageInfo {
    std::u16string base_name;
    std::optional<uint64_t> request_id;
};

inline std::u16string ToU16String(uint64_t value)
{
    if (value == 0) {
        return std::u16string(1, u'0');
    }

    std::u16string digits;
    while (value > 0) {
        const auto digit = static_cast<char16_t>(u'0' + (value % 10));
        digits.push_back(digit);
        value /= 10;
    }
    std::reverse(digits.begin(), digits.end());
    return digits;
}

inline CefString BuildRoutedMessageName(const CefString& base, uint64_t request_id)
{
    std::u16string routed = base.ToString16();
    routed.push_back(u':');
    auto id = ToU16String(request_id);
    routed.append(id.begin(), id.end());
    return CefString(routed);
}

inline CefString BuildRoutedMessageName(std::u16string_view base, uint64_t request_id)
{
    std::u16string routed(base);
    routed.push_back(u':');
    auto id = ToU16String(request_id);
    routed.append(id.begin(), id.end());
    return CefString(routed);
}

inline CefString BuildRoutedMessageName(const char16_t* base, uint64_t request_id)
{
    if (!base) {
        return BuildRoutedMessageName(std::u16string_view(), request_id);
    }
    std::u16string_view view(base, std::char_traits<char16_t>::length(base));
    return BuildRoutedMessageName(view, request_id);
}

inline RoutedMessageInfo ParseRoutedMessage(const CefString& name)
{
    RoutedMessageInfo info;
    info.base_name = name.ToString16();
    const auto size = info.base_name.size();
    if (size < 2) {
        return info;
    }

    size_t digit_start = size;
    while (digit_start > 0) {
        const char16_t ch = info.base_name[digit_start - 1];
        if (ch < u'0' || ch > u'9') {
            break;
        }
        --digit_start;
    }

    if (digit_start == size) {
        return info; // no digits at the end
    }

    if (digit_start == 0 || info.base_name[digit_start - 1] != u':') {
        return info; // suffix is part of the base name (e.g., Windows drive "C:")
    }

    uint64_t id = 0;
    for (size_t i = digit_start; i < size; ++i) {
        const char16_t ch = info.base_name[i];
        if (ch < u'0' || ch > u'9') {
            return info;
        }
        id = id * 10 + static_cast<uint64_t>(ch - u'0');
    }

    info.base_name.resize(digit_start - 1);
    info.request_id = id;
    return info;
}
} // namespace vortex::ui
