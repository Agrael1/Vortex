#include "platform.h"
#include <vortex/util/log.h>
#include <wisdom/wisdom_platform.hpp>
#include <SDL3/SDL_platform_defines.h>
#include <SDL3/SDL_video.h>


vortex::PlatformExtension::PlatformExtension()
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

vortex::PlatformExtension::~PlatformExtension()
{
}
