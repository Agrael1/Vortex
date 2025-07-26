#pragma once
#include <vortex/util/lib/reflect.h>
#include <vortex/util/log.h>
#include <charconv>
#include <DirectXMath.h>

namespace vortex {
template<typename T>
concept digit = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept string_type = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, char*>;

template<typename T>
struct vector_reflection {
    using basic_type = reflect::member_type<0, T>;
    static constexpr std::string_view name = reflect::type_name<T>();
    static constexpr uint32_t member_count = []() noexcept {
        uint32_t count = 0;
        reflect::for_each<T>([&count](auto&& member) {
            ++count;
        });
        return count;
    }();
};

// Traits for reflection types
template<typename T>
struct reflection_traits_base {
    // Base class for reflection traits, can be specialized for specific types
    static constexpr bool deserialize(T* obj, std::string_view data) noexcept
    {
        vortex::error("Deserialization not implemented for type {}", reflect::type_name(obj));
        return false;
    }
    static std::string serialize(const T& obj) noexcept
    {
        return ""; // Default serialization using format
    }
};

template<typename T>
struct reflection_traits : reflection_traits_base<T> {
};

template<digit I>
struct reflection_traits<I> : reflection_traits_base<I> {
    static constexpr bool deserialize(I* obj, std::string_view data) noexcept
    {
        // From chars
        auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), *obj);
        if (ec == std::errc()) {
            return true; // Success
        } else {
            vortex::error("Failed to deserialize integral or floating type {}: {}", reflect::type_name<I>(), reflect::enum_name(ec));
            return false; // Failure
        }
    }
};

template<string_type S>
struct reflection_traits<S> : reflection_traits_base<S> {
    static constexpr bool deserialize(S* obj, std::string_view data) noexcept
    {
        *obj = S{ data }; // Convert string_view to string
        return true; // Success
    }
    static std::string serialize(const S& obj) noexcept
    {
        return std::string(obj); // Convert string to string
    }
};

template<>
struct reflection_traits<bool> : reflection_traits_base<bool> {
    static constexpr bool deserialize(bool* obj, std::string_view data) noexcept
    {
        if (data == "true" || data != "0") {
            *obj = true;
        } else if (data == "false" || data == "0") {
            *obj = false;
        }
        return true;
    }
};

// Specialization for DirectX:: vector types
template<typename VT>
struct vector_traits {
    using base_type = decltype(VT::x);
    static constexpr size_t member_count = sizeof(VT) / sizeof(base_type);
    static void set_member(VT& obj, size_t index, base_type value) noexcept
    {
        if (index == 0) {
            obj.x = value;
            return;
        }
        if (index == 1) {
            obj.y = value;
            return;
        }
        if constexpr (member_count > 2) {
            if (index == 2) {
                obj.z = value;
            }
        }
        if constexpr (member_count > 3) {
            if (index == 3) {
                obj.w = value;
            }
        }
    }
    static base_type get_member(const VT& obj, size_t index) noexcept
    {
        if (index == 0) {
            return obj.x;
        }
        if (index == 1) {
            return obj.y;
        }
        if constexpr (member_count > 2) {
            if (index == 2) {
                return obj.z;
            }
        }
        if constexpr (member_count > 3) {
            if (index == 3) {
                return obj.w;
            }
        }
        return base_type{}; // Default value if index is out of bounds
    }
    static void desetialize_one(VT* obj, std::string_view data, size_t index) noexcept
    {
        base_type value{};
        if (reflection_traits<base_type>::deserialize(&value, data)) {
            set_member(*obj, index, value);
        } else {
            vortex::error("Failed to deserialize member {} of vector type {}", index, reflect::type_name<VT>());
        }
    }
};

template<size_t N>
constexpr std::array<std::string_view, N> split_string(std::string_view str, char delimiter) noexcept
{
    std::array<std::string_view, N> result{};
    size_t start = 0;
    size_t end = 0;
    size_t index = 0;
    while (end != std::string_view::npos && index < N) {
        end = str.find(delimiter, start);
        result[index++] = str.substr(start, end - start);
        result[index - 1].remove_prefix(std::min(result[index - 1].find_first_not_of(' '), result[index - 1].size()));
        start = end + 1;
    }
    return result;
}

template<typename T, typename... Args>
constexpr bool is_any_of_v = (std::is_same_v<T, Args> || ...);

template<typename T>
concept directx_vector = is_any_of_v<T, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMUINT2, DirectX::XMUINT3, DirectX::XMUINT4> ||
        is_any_of_v<T, DirectX::XMFLOAT2A, DirectX::XMFLOAT3A, DirectX::XMFLOAT4A>;

template<directx_vector V>
struct reflection_traits<V> : reflection_traits_base<V> {
    static constexpr bool deserialize(V* obj, std::string_view data) noexcept
    {
        using vref = vector_traits<V>;

        // Expecting format "[x,y]"
        if (data.size() < 5 || data.front() != '[' || data.back() != ']') {
            vortex::error("Failed to deserialize {}: Invalid format {}", reflect::type_name<V>(), data);
            return false;
        }
        std::string_view content = data.substr(1, data.size() - 2); // Remove brackets
        std::array<std::string_view, vref::member_count> values = split_string<vref::member_count>(content, ',');
        for (size_t i = 0; i < vref::member_count; ++i) {
            vref::desetialize_one(obj, values[i], i);
        }
        return true; // Success
    }
    static std::string serialize(V obj) noexcept
    {
        using vref = vector_traits<V>;
        std::string result = "[";
        for (size_t i = 0; i < vref::member_count; ++i) {
            if (i > 0) {
                result += ",";
            }
            result += std::to_string(vref::get_member(obj, i));
        }
        result += "]";
        return result; // Serialize as "[x,y,z,w]"
    }
};

using notifier_callback = std::function<void(uint32_t index, std::string_view data)>;
} // namespace vortex