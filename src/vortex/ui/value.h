#pragma once
#include <include/cef_values.h>
#include <string_view>
#include <vortex/util/log.h>

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
        vortex::warn("Unsupported type for CefListValue: {}", typeid(T).name());
        list.SetNull(index);
    }
};

// Specializations for specific types
template<>
struct value_traits<bool> {
    static void add_value(CefListValue& list, size_t index, bool value)
    {
        list.SetBool(index, value);
    }
};

template<>
struct value_traits<int> {
    static void add_value(CefListValue& list, size_t index, int value)
    {
        list.SetInt(index, value);
    }
};

template<>
struct value_traits<double> {
    static void add_value(CefListValue& list, size_t index, double value)
    {
        list.SetDouble(index, value);
    }
};

template<>
struct value_traits<std::u16string_view> {
    static void add_value(CefListValue& list, size_t index, std::u16string_view value)
    {
        list.SetString(index, CefString(value.data(), value.size(), false));
    }
};

template<>
struct value_traits<std::string_view> {
    static void add_value(CefListValue& list, size_t index, std::string_view value)
    {
        list.SetString(index, CefString(value.data(), value.size()));
    }
};

template<>
struct value_traits<CefRefPtr<CefValue>> {
    static void add_value(CefListValue& list, size_t index, CefRefPtr<CefValue> value)
    {
        list.SetValue(index, value);
    }
};
} // namespace vortex::ui