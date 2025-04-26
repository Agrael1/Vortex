#pragma once
#include <unordered_map>
#include <string_view>
#include <wisdom/generated/api/api.hpp>

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