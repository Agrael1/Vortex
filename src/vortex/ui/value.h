#pragma once
#include <include/cef_values.h>
#include <include/cef_v8.h>
#include <string_view>
#include <vortex/util/log.h>
#include <vortex/util/reflection.h>

namespace vortex::ui {
struct string_traits {
    static CefString to_cef(std::u16string_view str)
    {
        return CefString(str.data(), str.size(), false);
    }
    static CefString to_cef(std::string_view str)
    {
        return CefString(str.data(), str.size());
    }
};

template<typename T>
struct value_traits {
    static void add_value(CefListValue& list, size_t index, T&& value)
    {
        static_assert(false, "Unsupported type for CefListValue");
        vortex::warn("Unsupported type for CefListValue: {}", typeid(T).name());
        list.SetNull(index);
    }
    static void extract_value(T& out, CefListValue& list, size_t index)
    {
        static_assert(false, "Unsupported type for CefListValue extraction");
        vortex::warn("Unsupported type for CefListValue extraction: {}", typeid(T).name());
        out = T{};
    }
};

// Specializations for specific types
template<>
struct value_traits<bool> {
    static void add_value(CefListValue& list, size_t index, bool value)
    {
        list.SetBool(index, value);
    }
    static bool extract_value(bool& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_BOOL) {
            return false;
        }

        out = list.GetBool(index);
        return true;
    }
};

template<>
struct value_traits<int> {
    static void add_value(CefListValue& list, size_t index, int value)
    {
        list.SetInt(index, value);
    }
    static bool extract_value(int& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_INT) {
            return false;
        }
        out = list.GetInt(index);
        return true;
    }
};

template<>
struct value_traits<double> {
    static void add_value(CefListValue& list, size_t index, double value)
    {
        list.SetDouble(index, value);
    }
    static bool extract_value(double& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_DOUBLE) {
            return false;
        }
        out = list.GetDouble(index);
        return true;
    }
};

template<>
struct value_traits<uintptr_t> {
    static void add_value(CefListValue& list, size_t index, uintptr_t value)
    {
        list.SetDouble(index, std::bit_cast<double>(value));
    }
    static bool extract_value(uintptr_t& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_DOUBLE) {
            return false;
        }
        out = std::bit_cast<uintptr_t>(list.GetDouble(index));
        return true;
    }
};

template<>
struct value_traits<std::u16string_view> {
    static void add_value(CefListValue& list, size_t index, std::u16string_view value)
    {
        list.SetString(index, CefString(value.data(), value.size(), false));
    }
    static bool extract_value(std::u16string_view& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_STRING) {
            return false;
        }
        auto str = list.GetString(index);
        out = std::u16string_view(str.c_str(), str.length());
        return true;
    }
};

template<string_type S>
struct value_traits<S> {
    static void add_value(CefListValue& list, size_t index, std::string_view value)
    {
        list.SetString(index, CefString(value.data(), value.size()));
    }
    static bool extract_value(std::string& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_STRING) {
            return false;
        }
        auto str = list.GetString(index);
        out = str.ToString();
        return true;
    }
};

template<>
struct value_traits<CefRefPtr<CefValue>> {
    static void add_value(CefListValue& list, size_t index, CefRefPtr<CefValue> value)
    {
        list.SetValue(index, value);
    }
    static bool extract_value(CefRefPtr<CefValue>& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) == VTYPE_INVALID) {
            return false;
        }
        out = list.GetValue(index);
        return true;
    }
};

template<>
struct value_traits<CefRefPtr<CefDictionaryValue>> {
    static void add_value(CefListValue& list, size_t index, CefRefPtr<CefDictionaryValue> value)
    {
        list.SetDictionary(index, value);
    }
    static bool extract_value(CefRefPtr<CefDictionaryValue>& out, CefListValue& list, size_t index)
    {
        if (list.GetType(index) != VTYPE_DICTIONARY) {
            return false;
        }
        out = list.GetDictionary(index);
        return true;
    }
};

template<CefValueType type>
struct v8_value_traits {

    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index index)
    {
        return CefV8Value::CreateNull();
    }
};

template<template<CefValueType> typename Exec, typename Container, typename Index>
inline decltype(auto) bridge(CefRefPtr<Container> container, Index&& index)
{
    auto type = container->GetType(index);
    switch (type) {
    case CefValueType::VTYPE_BOOL:
        return Exec<CefValueType::VTYPE_BOOL>::convert(std::move(container), std::move(index));
    case CefValueType::VTYPE_INT:
        return Exec<CefValueType::VTYPE_INT>::convert(std::move(container), std::move(index));
    case CefValueType::VTYPE_DOUBLE:
        return Exec<CefValueType::VTYPE_DOUBLE>::convert(std::move(container), std::move(index));
    case CefValueType::VTYPE_STRING:
        return Exec<CefValueType::VTYPE_STRING>::convert(std::move(container), std::move(index));
    case CefValueType::VTYPE_DICTIONARY:
        return Exec<CefValueType::VTYPE_DICTIONARY>::convert(std::move(container), std::move(index));
    case CefValueType::VTYPE_LIST:
        return Exec<CefValueType::VTYPE_LIST>::convert(std::move(container), std::move(index));
    default:
        vortex::warn("Unsupported CefValueType: {}", reflect::enum_name(type));
    case CefValueType::VTYPE_NULL:
        return Exec<CefValueType::VTYPE_INVALID>::convert(std::move(container), std::move(index));
    }
}

template<>
struct v8_value_traits<CefValueType::VTYPE_BOOL> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        return CefV8Value::CreateBool(container->GetBool(index));
    }
};
template<>
struct v8_value_traits<CefValueType::VTYPE_INT> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        return CefV8Value::CreateInt(container->GetInt(index));
    }
};
template<>
struct v8_value_traits<CefValueType::VTYPE_DOUBLE> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        return CefV8Value::CreateDouble(container->GetDouble(index));
    }
};
template<>
struct v8_value_traits<CefValueType::VTYPE_STRING> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        return CefV8Value::CreateString(container->GetString(index));
    }
};
template<>
struct v8_value_traits<CefValueType::VTYPE_DICTIONARY> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        auto dict = container->GetDictionary(index);
        auto obj = CefV8Value::CreateObject(nullptr, nullptr);
        if (dict) {
            std::vector<CefString> keys;
            keys.reserve(dict->GetSize());
            dict->GetKeys(keys);
            for (const auto& key : keys) {
                obj->SetValue(key, bridge<v8_value_traits>(dict, key), V8_PROPERTY_ATTRIBUTE_NONE);
            }
        }
        return obj;
    }
};
template<>
struct v8_value_traits<CefValueType::VTYPE_LIST> {
    template<typename Container, typename Index>
    static CefRefPtr<CefV8Value> convert(CefRefPtr<Container> container, Index&& index)
    {
        auto list = container->GetList(index);
        auto arr = CefV8Value::CreateArray(list->GetSize());
        for (int i = 0; i < list->GetSize(); ++i) {
            arr->SetValue(i, bridge<v8_value_traits>(list, i));
        }
        return arr;
    }
};

template<typename... Args>
inline auto extract_arguments(CefListValue& list, bool& success)
{
    std::tuple<Args...> result;
    size_t index = 0;

    // C++23: Enhanced fold expressions with index sequence
    success = [&]<std::size_t... I>(std::index_sequence<I...>) {
        return ([&] {
            return value_traits<Args>::extract_value(std::get<I>(result), list, index++);
        }() && ...);
    }(std::index_sequence_for<Args...>{});

    return result;
}
} // namespace vortex::ui