module;
#include <memory>
#include <SDL3/SDL_platform_defines.h>
export module vortex.platform;

import wisdom;
import wisdom.platform;
import vortex.log;

namespace vortex {

// Use SDL to detect platform
export class PlatformExtension
{
public:
    enum class Selector {
        None,
        Windows,
        X11,
        Wayland
    };

public:
    PlatformExtension()
    {
        current = Selector::None;
        platform = {};
#if defined(SDL_PLATFORM_WIN32)
        platform = std::make_unique<wis::platform::WindowsExtension>();
        current = Selector::Windows;
        vortex::info("Windows platform detected");
#elif defined(SDL_PLATFORM_LINUX)
        const char* platform_name = SDL_GetCurrentVideoDriver();
        if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
            platform = std::make_unique<wis::platform::X11Extension>();
            current = Selector::X11;
            vortex::info("X11 platform detected");
        } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
            platform = std::make_unique<wis::platform::WaylandExtension>();
            current = Selector::Wayland;
            vortex::info("Wayland platform detected");
        } else {
            vortex::error("Unknown platform: {}", platform_name);
        }
#endif
    }

public:
    template<typename Self>
    auto* GetExtension(this Self&& self) noexcept
    {
        using propagate_const = std::conditional_t<
                std::is_const_v<std::remove_reference_t<Self>>,
                const wis::FactoryExtension*,
                wis::FactoryExtension*>;

        return const_cast<propagate_const>(self.platform.get());
    }

public:
    Selector current = Selector::None;
    std::unique_ptr<wis::FactoryExtension> platform;
};
} // namespace vortex