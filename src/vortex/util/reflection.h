#pragma once
#include <vortex/util/lib/reflect.h>
#include <vortex/util/log.h>
#include <charconv>

namespace vortex {
template<typename T>
concept digit = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept string_type = std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, char*>;

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




template<typename T>
struct static_reflection {
    struct member {
        using deserizalize_func = bool (*)(void* obj, std::string_view data); // Not extra safe, but simple

        std::string_view name;
        deserizalize_func deserialize = nullptr;
        ptrdiff_t offset = 0;
    };

    static constexpr std::string_view name = reflect::type_name<T>();
    static constexpr uint32_t member_count = []() noexcept {
        uint32_t count = 0;
        reflect::for_each<T>([&count](auto&& member) {
            ++count;
        });
        return count;
    }();

    static constexpr std::array<member, member_count> members = []() noexcept {
        std::array<member, member_count> arr{};
        uint32_t index = 0;
        reflect::for_each<T>([&arr, &index](const auto member) {
            using member_type = reflect::member_type<member, T>;
            arr[index++] = {
                .name = reflect::member_name<member, T>(),
                .deserialize = [](void* obj, std::string_view data) -> bool {
                    return reflection_traits<member_type>::deserialize(std::bit_cast<member_type*>(obj), data);
                },
                .offset = reflect::offset_of<member, T>()
            };
        });
        return arr;
    }();
};

using notifier_callback = std::function<void(uint32_t index, std::string_view data)>;
} // namespace vortex