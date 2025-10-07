#pragma once
#include <vortex/properties/type_traits.h>
#include <vortex/util/interp/vector.h>
#include <any>
#include <functional>
#include <cmath>

namespace vortex {
// Base interpolation trait
template<typename T>
struct interpolation_traits {
    // Default implementation - no interpolation support
    static constexpr bool is_interpolatable = false;

    static T interpolate(const T& from, const T& to, float t)
    {
        return t < 0.5f ? from : to; // Step function
    }
};

// void type specialization
template<>
struct interpolation_traits<void> {
    static constexpr bool is_interpolatable = false;
};

// Arithmetic types interpolation
template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template<arithmetic T>
struct interpolation_traits<T> {
    static constexpr bool is_interpolatable = true;

    static T interpolate(T from, T to, float t) { return std::lerp(from, to, t); }
};

template<directx_vector_type T>
struct interpolation_traits<T> {
    static constexpr bool is_interpolatable = true;

    static T interpolate(T from, T to, float t)
    {
        DirectX::XMVECTOR v_from = vector_ops<T>::to_vector(from);
        DirectX::XMVECTOR v_to = vector_ops<T>::to_vector(to);
        DirectX::XMVECTOR v_result = DirectX::XMVectorLerp(v_from, v_to, t);
        return vector_ops<T>::from_vector(v_result);
    }
};

// Matrix interpolation
template<>
struct interpolation_traits<DirectX::XMFLOAT4X4> {
    static constexpr bool is_interpolatable = true;

    static DirectX::XMFLOAT4X4 interpolate(const DirectX::XMFLOAT4X4& from,
                                           const DirectX::XMFLOAT4X4& to,
                                           float eased_t)
    {
        DirectX::XMMATRIX m1 = DirectX::XMLoadFloat4x4(&from);
        DirectX::XMMATRIX m2 = DirectX::XMLoadFloat4x4(&to);

        // Decompose matrices
        DirectX::XMVECTOR scale1, rot1, trans1;
        DirectX::XMVECTOR scale2, rot2, trans2;

        DirectX::XMMatrixDecompose(&scale1, &rot1, &trans1, m1);
        DirectX::XMMatrixDecompose(&scale2, &rot2, &trans2, m2);

        // Interpolate components
        DirectX::XMVECTOR scale_result = DirectX::XMVectorLerp(scale1, scale2, eased_t);
        DirectX::XMVECTOR rot_result = DirectX::XMQuaternionSlerp(rot1, rot2, eased_t);
        DirectX::XMVECTOR trans_result = DirectX::XMVectorLerp(trans1, trans2, eased_t);

        // Reconstruct matrix
        DirectX::XMMATRIX result = DirectX::XMMatrixAffineTransformation(scale_result,
                                                                         DirectX::XMVectorZero(),
                                                                         rot_result,
                                                                         trans_result);

        DirectX::XMFLOAT4X4 out;
        DirectX::XMStoreFloat4x4(&out, result);
        return out;
    }
};

template<typename T>
constexpr bool is_interpolatable_v = interpolation_traits<T>::is_interpolatable;

template<PropertyType type>
struct interpolate_property {
    using traits = interpolation_traits<typename type_traits<type>::type>;
};
template<>
struct interpolate_property<vortex::PropertyType::Void> {
    using traits = interpolation_traits<void>;
};
template<>
struct interpolate_property<PropertyType::Quaternion> {
    struct traits {
        static constexpr bool is_interpolatable = true;
        using type = DirectX::XMFLOAT4;
        static type interpolate(type from, type to, float t)
        {
            DirectX::XMVECTOR v_from = DirectX::XMLoadFloat4(&from);
            DirectX::XMVECTOR v_to = DirectX::XMLoadFloat4(&to);
            DirectX::XMVECTOR v_result = DirectX::XMQuaternionSlerp(v_from, v_to, t);
            type result;
            DirectX::XMStoreFloat4(&result, v_result);
            return result;
        }
    };
};

template<PropertyType type>
struct interpolate_property_executor {
    using traits = typename interpolate_property<type>::traits;
    using value_type = typename type_traits<type>::type;
    static PropertyValue operator()(const PropertyValue& from, const PropertyValue& to, float t)
    {
        return traits::interpolate(std::get<value_type>(from), std::get<value_type>(to), t);
    }
};
template<>
struct interpolate_property_executor<PropertyType::Void> {
    static PropertyValue operator()(const PropertyValue& from, const PropertyValue& to, float t)
    {
        return {};
    }
};

} // namespace vortex