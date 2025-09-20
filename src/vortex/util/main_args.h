#pragma once
#include <span>
#include <string_view>


namespace vortex {
struct MainArgs {
    bool headless = false;
};

inline MainArgs ParseArgs(std::span<std::string_view> args) noexcept
{
    MainArgs result;
    for (const auto& arg : args) {
        if (arg == "--headless") {
            result.headless = true;
        }
    }
    return result;
}
} // namespace vortex