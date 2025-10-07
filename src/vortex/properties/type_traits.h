// Generated type traits
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>
#include <vortex/util/reflection.h>
#include <vortex/util/log.h>
#include <DirectXMath.h>
#include <variant>

namespace vortex {
enum class PropertyType {
    Bool,
    U16,
    Void,
    U8,
    U32,
    F32x2,
    U64,
    I8,
    U32x2,
    I16,
    I32,
    I64,
    F32,
    F64,
    F32x3,
    F32x4,
    Sizei,
    I32x2,
    Size,
    I32x3,
    I32x4,
    U32x3,
    Quaternion,
    U32x4,
    U8string,
    U16string,
    Path,
    Color,
    Rect,
    Sizeu,
    Point2d,
    Point3d,
    Point,
    Matrix,
};

using PropertyValue = std::variant<std::monostate, // index 0: empty state
                                   bool,
                                   DirectX::XMUINT4,
                                   uint16_t,
                                   int8_t,
                                   DirectX::XMFLOAT3,
                                   uint64_t,
                                   uint8_t,
                                   DirectX::XMUINT2,
                                   uint32_t,
                                   DirectX::XMFLOAT2,
                                   int16_t,
                                   int32_t,
                                   int64_t,
                                   float,
                                   double,
                                   DirectX::XMFLOAT4,
                                   DirectX::XMINT2,
                                   DirectX::XMINT3,
                                   DirectX::XMINT4,
                                   DirectX::XMUINT3,
                                   std::string,
                                   std::wstring,
                                   DirectX::XMFLOAT4X4>;
template<PropertyType T>
struct type_traits;

template<>
struct type_traits<PropertyType::Bool> {
    using type = bool;
};
template<>
struct type_traits<PropertyType::U16> {
    using type = uint16_t;
};
template<>
struct type_traits<PropertyType::Void> {
    using type = void;
};
template<>
struct type_traits<PropertyType::U8> {
    using type = uint8_t;
};
template<>
struct type_traits<PropertyType::U32> {
    using type = uint32_t;
};
template<>
struct type_traits<PropertyType::F32x2> {
    using type = DirectX::XMFLOAT2;
};
template<>
struct type_traits<PropertyType::U64> {
    using type = uint64_t;
};
template<>
struct type_traits<PropertyType::I8> {
    using type = int8_t;
};
template<>
struct type_traits<PropertyType::U32x2> {
    using type = DirectX::XMUINT2;
};
template<>
struct type_traits<PropertyType::I16> {
    using type = int16_t;
};
template<>
struct type_traits<PropertyType::I32> {
    using type = int32_t;
};
template<>
struct type_traits<PropertyType::I64> {
    using type = int64_t;
};
template<>
struct type_traits<PropertyType::F32> {
    using type = float;
};
template<>
struct type_traits<PropertyType::F64> {
    using type = double;
};
template<>
struct type_traits<PropertyType::F32x3> {
    using type = DirectX::XMFLOAT3;
};
template<>
struct type_traits<PropertyType::F32x4> {
    using type = DirectX::XMFLOAT4;
};
template<>
struct type_traits<PropertyType::Sizei> {
    using type = DirectX::XMINT2;
};
template<>
struct type_traits<PropertyType::I32x2> {
    using type = DirectX::XMINT2;
};
template<>
struct type_traits<PropertyType::Size> {
    using type = DirectX::XMFLOAT2;
};
template<>
struct type_traits<PropertyType::I32x3> {
    using type = DirectX::XMINT3;
};
template<>
struct type_traits<PropertyType::I32x4> {
    using type = DirectX::XMINT4;
};
template<>
struct type_traits<PropertyType::U32x3> {
    using type = DirectX::XMUINT3;
};
template<>
struct type_traits<PropertyType::Quaternion> {
    using type = DirectX::XMFLOAT4;
};
template<>
struct type_traits<PropertyType::U32x4> {
    using type = DirectX::XMUINT4;
};
template<>
struct type_traits<PropertyType::U8string> {
    using type = std::string;
};
template<>
struct type_traits<PropertyType::U16string> {
    using type = std::wstring;
};
template<>
struct type_traits<PropertyType::Path> {
    using type = std::string;
};
template<>
struct type_traits<PropertyType::Color> {
    using type = DirectX::XMFLOAT4;
};
template<>
struct type_traits<PropertyType::Rect> {
    using type = DirectX::XMFLOAT4;
};
template<>
struct type_traits<PropertyType::Sizeu> {
    using type = DirectX::XMUINT2;
};
template<>
struct type_traits<PropertyType::Point2d> {
    using type = DirectX::XMFLOAT2;
};
template<>
struct type_traits<PropertyType::Point3d> {
    using type = DirectX::XMFLOAT3;
};
template<>
struct type_traits<PropertyType::Point> {
    using type = DirectX::XMFLOAT2;
};
template<>
struct type_traits<PropertyType::Matrix> {
    using type = DirectX::XMFLOAT4X4;
};

template<template<PropertyType> typename Exec, typename... Args>
decltype(auto) bridge(PropertyType type, Args&&... args)
{
    switch (type) {
    case PropertyType::Bool:
        return Exec<PropertyType::Bool>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U16:
        return Exec<PropertyType::U16>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Void:
        return Exec<PropertyType::Void>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U8:
        return Exec<PropertyType::U8>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U32:
        return Exec<PropertyType::U32>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::F32x2:
        return Exec<PropertyType::F32x2>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U64:
        return Exec<PropertyType::U64>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I8:
        return Exec<PropertyType::I8>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U32x2:
        return Exec<PropertyType::U32x2>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I16:
        return Exec<PropertyType::I16>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I32:
        return Exec<PropertyType::I32>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I64:
        return Exec<PropertyType::I64>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::F32:
        return Exec<PropertyType::F32>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::F64:
        return Exec<PropertyType::F64>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::F32x3:
        return Exec<PropertyType::F32x3>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::F32x4:
        return Exec<PropertyType::F32x4>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Sizei:
        return Exec<PropertyType::Sizei>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I32x2:
        return Exec<PropertyType::I32x2>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Size:
        return Exec<PropertyType::Size>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I32x3:
        return Exec<PropertyType::I32x3>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::I32x4:
        return Exec<PropertyType::I32x4>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U32x3:
        return Exec<PropertyType::U32x3>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Quaternion:
        return Exec<PropertyType::Quaternion>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U32x4:
        return Exec<PropertyType::U32x4>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U8string:
        return Exec<PropertyType::U8string>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::U16string:
        return Exec<PropertyType::U16string>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Path:
        return Exec<PropertyType::Path>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Color:
        return Exec<PropertyType::Color>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Rect:
        return Exec<PropertyType::Rect>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Sizeu:
        return Exec<PropertyType::Sizeu>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Point2d:
        return Exec<PropertyType::Point2d>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Point3d:
        return Exec<PropertyType::Point3d>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Point:
        return Exec<PropertyType::Point>{}(std::forward<Args>(args)...);
        break;
    case PropertyType::Matrix:
        return Exec<PropertyType::Matrix>{}(std::forward<Args>(args)...);
        break;
    default:
        vortex::warn("Unsupported CefValueType: {}", reflect::enum_name(type));
        return Exec<PropertyType::Void>{}(std::forward<Args>(args)...);
    }
}

} // namespace vortex
