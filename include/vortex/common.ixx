module;
#include <unordered_map>
#include <string_view>
export module vortex.common;

import wisdom;

namespace vortex {
export inline bool success(wis::Result result) noexcept
{
    return int(result.status) >= 0;
}

export struct string_hash : public std::hash<std::string_view> {
    using is_transparent = void;
};
export struct string_equal : public std::equal_to<std::string_view> {
    using is_transparent = void;
};
}