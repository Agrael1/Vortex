#pragma once
#include <memory>
#include <wisdom/wisdom.hpp>

namespace vortex {

// Use SDL to detect platform
class PlatformExtension
{
public:
    enum class Selector {
        None,
        Windows,
        X11,
        Wayland
    };

public:
    PlatformExtension();
    ~PlatformExtension();

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