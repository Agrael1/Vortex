#pragma once
#include <DirectXMath.h>
#include <concepts>

namespace vortex {
template<typename T>
concept directx_vector_type = std::same_as<T, DirectX::XMFLOAT2> ||
        std::same_as<T, DirectX::XMFLOAT3> || std::same_as<T, DirectX::XMFLOAT4> ||
        std::same_as<T, DirectX::XMUINT2> || std::same_as<T, DirectX::XMUINT3> ||
        std::same_as<T, DirectX::XMUINT4> || std::same_as<T, DirectX::XMINT2> ||
        std::same_as<T, DirectX::XMINT3> || std::same_as<T, DirectX::XMINT4>;

template<typename VT>
struct vector_ops;

template<>
struct vector_ops<DirectX::XMFLOAT2> {
    using base_type = float;
    static DirectX::XMVECTOR to_vector(DirectX::XMFLOAT2 v) noexcept
    {
        return DirectX::XMLoadFloat2(&v);
    }
    static DirectX::XMFLOAT2 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMFLOAT2 result;
        DirectX::XMStoreFloat2(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMFLOAT3> {
    using base_type = float;
    static DirectX::XMVECTOR to_vector(DirectX::XMFLOAT3 v) noexcept
    {
        return DirectX::XMLoadFloat3(&v);
    }
    static DirectX::XMFLOAT3 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMFLOAT4> {
    using base_type = float;
    static DirectX::XMVECTOR to_vector(DirectX::XMFLOAT4 v) noexcept
    {
        return DirectX::XMLoadFloat4(&v);
    }
    static DirectX::XMFLOAT4 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMFLOAT4 result;
        DirectX::XMStoreFloat4(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMINT2> {
    using base_type = int32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMINT2 v) noexcept
    {
        return DirectX::XMLoadSInt2(&v);
    }
    static DirectX::XMINT2 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMINT2 result;
        DirectX::XMStoreSInt2(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMINT3> {
    using base_type = int32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMINT3 v) noexcept
    {
        return DirectX::XMLoadSInt3(&v);
    }
    static DirectX::XMINT3 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMINT3 result;
        DirectX::XMStoreSInt3(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMINT4> {
    using base_type = int32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMINT4 v) noexcept
    {
        return DirectX::XMLoadSInt4(&v);
    }
    static DirectX::XMINT4 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMINT4 result;
        DirectX::XMStoreSInt4(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMUINT2> {
    using base_type = uint32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMUINT2 v) noexcept
    {
        return DirectX::XMLoadUInt2(&v);
    }
    static DirectX::XMUINT2 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMUINT2 result;
        DirectX::XMStoreUInt2(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMUINT3> {
    using base_type = uint32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMUINT3 v) noexcept
    {
        return DirectX::XMLoadUInt3(&v);
    }
    static DirectX::XMUINT3 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMUINT3 result;
        DirectX::XMStoreUInt3(&result, v);
        return result;
    }
};
template<>
struct vector_ops<DirectX::XMUINT4> {
    using base_type = uint32_t;
    static DirectX::XMVECTOR to_vector(DirectX::XMUINT4 v) noexcept
    {
        return DirectX::XMLoadUInt4(&v);
    }
    static DirectX::XMUINT4 from_vector(DirectX::XMVECTOR v) noexcept
    {
        DirectX::XMUINT4 result;
        DirectX::XMStoreUInt4(&result, v);
        return result;
    }
};
} // namespace vortex