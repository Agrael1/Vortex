#pragma once
#include <numbers>
#include <cmath>

namespace vortex {
enum class EaseType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce
};

template<EaseType T>
struct easing_function;

template<>
struct easing_function<EaseType::Linear> {
    static constexpr float execute(float t) { return t; }
};
template<>
struct easing_function<EaseType::EaseIn> {
    static constexpr float execute(float t) { return t * t; }
};
template<>
struct easing_function<EaseType::EaseOut> {
    static constexpr float execute(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
};
template<>
struct easing_function<EaseType::EaseInOut> {
    static constexpr float execute(float t)
    {
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }
};
template<>
struct easing_function<EaseType::EaseInBack> {
    static constexpr float execute(float t)
    {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        return c3 * t * t * t - c1 * t * t;
    }
};
template<>
struct easing_function<EaseType::EaseOutBack> {
    static constexpr float execute(float t)
    {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        auto mt = t - 1.0f;
        auto mt2 = mt * mt;
        return 1.0f + c3 * mt * mt2 + c1 * mt2;
    }
};
template<>
struct easing_function<EaseType::EaseInOutBack> {
    static constexpr float execute(float t)
    {
        constexpr float c1 = 1.70158f;
        constexpr float c2 = c1 * 1.525f;
        auto mt = t * 2.0f;
        auto mt2 = mt * mt;
        auto mtm2 = mt - 2.0f;
        return t < 0.5f ? (mt2 * ((c2 + 1.0f) * mt - c2)) / 2.0f
                        : (mtm2 * mtm2 * ((c2 + 1.0f) * mtm2 + c2) + 2.0f) / 2.0f;
    }
};
template<>
struct easing_function<EaseType::EaseInElastic> {
    static float execute(float t)
    {
        constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
        return t == 0.0f    ? 0.0f
                : t == 1.0f ? 1.0f
                            : -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * c4);
    }
};
template<>
struct easing_function<EaseType::EaseOutElastic> {
    static float execute(float t)
    {
        constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
        return t == 0.0f    ? 0.0f
                : t == 1.0f ? 1.0f
                            : std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * c4) + 1.0f;
    }
};
template<>
struct easing_function<EaseType::EaseInOutElastic> {
    static float execute(float t)
    {
        constexpr float c5 = (2.0f * std::numbers::pi_v<float>) / 4.5f;
        return t == 0.0f    ? 0.0f
                : t == 1.0f ? 1.0f
                : t < 0.5f
                ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f
                : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) /
                                2.0f +
                        1.0f;
    }
};
template<>
struct easing_function<EaseType::EaseOutBounce> {
    static float execute(float t)
    {
        constexpr float n1 = 7.5625f;
        constexpr float d1 = 2.75f;
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            auto mt = t - 1.5f / d1;
            return n1 * mt * mt + 0.75f;
        } else if (t < 2.5f / d1) {
            auto mt = t - 2.25f / d1;
            return n1 * mt * mt + 0.9375f;
        } else {
            auto mt = t - 2.625f / d1;
            return n1 * mt * mt + 0.984375f;
        }
    }
};
template<>
struct easing_function<EaseType::EaseInBounce> {
    static float execute(float t)
    {
        return 1.0f - easing_function<EaseType::EaseOutBounce>::execute(1.0f - t);
    }
};
template<>
struct easing_function<EaseType::EaseInOutBounce> {
    static float execute(float t)
    {
        return t < 0.5f
                ? (1.0f - easing_function<EaseType::EaseOutBounce>::execute(1.0f - 2.0f * t)) / 2.0f
                : (easing_function<EaseType::EaseOutBounce>::execute(2.0f * t - 1.0f)) / 2.0f +
                        0.5f;
    }
};

using easing_f = float (*)(float);

template<EaseType T>
constexpr easing_f easing_v = easing_function<T>::execute;

// Get easing function by type
inline easing_f get_easing_function(EaseType type)
{
    switch (type) {
    default:
    case EaseType::Linear:
        return easing_v<EaseType::Linear>;
    case EaseType::EaseIn:
        return easing_v<EaseType::EaseIn>;
    case EaseType::EaseOut:
        return easing_v<EaseType::EaseOut>;
    case EaseType::EaseInOut:
        return easing_v<EaseType::EaseInOut>;
    case EaseType::EaseInBack:
        return easing_v<EaseType::EaseInBack>;
    case EaseType::EaseOutBack:
        return easing_v<EaseType::EaseOutBack>;
    case EaseType::EaseInOutBack:
        return easing_v<EaseType::EaseInOutBack>;
    case EaseType::EaseInElastic:
        return easing_v<EaseType::EaseInElastic>;
    case EaseType::EaseOutElastic:
        return easing_v<EaseType::EaseOutElastic>;
    case EaseType::EaseInOutElastic:
        return easing_v<EaseType::EaseInOutElastic>;
    case EaseType::EaseInBounce:
        return easing_v<EaseType::EaseInBounce>;
    case EaseType::EaseOutBounce:
        return easing_v<EaseType::EaseOutBounce>;
    case EaseType::EaseInOutBounce:
        return easing_v<EaseType::EaseInOutBounce>;
    }
}
} // namespace vortex