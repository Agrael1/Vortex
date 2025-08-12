#pragma once
#include <type_traits>
#include <memory>

namespace vortex {
namespace detail {
template<typename Type>
inline constexpr bool is_handle_or_pointer_v = std::is_pointer_v<Type> || std::is_integral_v<Type>; // Assuming handles are integral types, like file descriptors or similar.
}

// Traits to validate the type for unique_any.
template<typename Type>
struct unique_any_traits {
    static constexpr bool is_valid(const Type& val) noexcept
    {
        // use bool conversion to check if Type is valid
        if constexpr (std::is_convertible_v<Type, bool>) {
            return static_cast<bool>(val);
        }
        return true; // Default to true if Type is not convertible to bool.
    }
};

template<typename Type, auto Cleanup>
struct unique_any {
public:
    using value_type = Type; // No decay, so this is the actual type.
    using get_type = std::conditional_t<detail::is_handle_or_pointer_v<value_type>, value_type, value_type&>; // Pointer if handle or pointer, reference otherwise.
    using traits_type = unique_any_traits<value_type>; // Type traits for validation.

    template<typename... Args>
    constexpr explicit unique_any(Args&&... args) noexcept
        : _storage{ std::forward<Args>(args)... }
    {
    }
    unique_any(const unique_any&) = delete;
    unique_any& operator=(const unique_any&) = delete;
    constexpr unique_any(unique_any&& other) noexcept
        : _storage(std::exchange(other._storage, value_type{})) // Move the storage from the other object.
    {
    }
    constexpr unique_any& operator=(unique_any&& other) noexcept
    {
        if (this != &other) {
            reset(); // Cleanup the current object before moving.
            _storage = std::exchange(other._storage, value_type{}); // Move the storage from the other object.
        }
        return *this;
    }
    constexpr ~unique_any() noexcept
    {
        clear_unsafe(); // No need to reset, just cleanup the storage.
    }

public:
    /// @brief Returns a reference to the stored value.
    /// @return Reference to the stored value.
    constexpr get_type get() noexcept
    {
        return _storage;
    }

    /// @brief Returns a const reference to the stored value.
    /// @return Const reference to the stored value.
    constexpr const get_type get() const noexcept
    {
        return _storage;
    }

    /// @brief Returns a pointer to the storage for initialization by external code.
    /// @return Pointer to the storage after cleanup.
    /// @note This function clears current storage before returning the pointer.
    /// @note Primarily intended for output parameters and initialization patterns.
    /// @warning After calling put(), the storage is in a default-constructed state.
    constexpr value_type* put() noexcept
    {
        reset(); // Cleanup the current storage.
        return &_storage; // Return a pointer to the storage.
    }

    /// @brief Performs cleanup on the current storage if it is valid.
    /// @note This function calls the cleanup function but does NOT reset/reconstruct the storage.
    /// @note The cleanup function may modify the stored value in-place to mark it as invalid.
    /// @warning After calling clear_unsafe(), the storage remains in whatever state the cleanup
    ///          function left it in. For proper cleanup and reset, use reset() instead.
    /// @warning Do not access the stored value after clear_unsafe() unless you know the cleanup
    ///          function's behavior. The value may be in an undefined or invalid state.
    /// @see reset() for cleanup followed by reconstruction with new values.
    constexpr void clear_unsafe() noexcept
    {
        if (traits_type::is_valid(_storage)) {
            do_cleanup(&_storage); // Cleanup the current storage.
        }
    }

    /// @brief Resets the storage with new arguments.
    /// @param ...args Arguments to construct the new value in storage.
    template<typename... Args>
    constexpr void reset(Args&&... args) noexcept
    {
        clear_unsafe(); // Cleanup the current storage.

        // if trivially destructible, we can use placement new directly.
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            std::destroy_at(&_storage); // Clean up the current storage.
        }
        std::construct_at(&_storage, std::forward<Args>(args)...); // Construct the new value in storage.
    }

    /// @brief Checks if the storage is valid.
    /// @return True if the storage is valid, false otherwise.
    constexpr bool is_valid() const noexcept
    {
        return traits_type::is_valid(_storage); // Check if the storage is valid.
    }

    /// @brief Checks if the storage is valid.
    /// @return True if the storage is valid, false otherwise.
    constexpr explicit operator bool() const noexcept
    {
        return is_valid(); // Check if the storage is valid.
    }

    constexpr auto operator->() noexcept
    {
        if constexpr (!std::is_pointer_v<value_type>) {
            return std::addressof(_storage); // Return a pointer to the stored value.
        } else {
            return get(); // Return a pointer to the stored value.
        }
    }
    constexpr auto operator->() const noexcept
    {
        if constexpr (!std::is_pointer_v<value_type>) {
            return std::addressof(_storage); // Return a const pointer to the stored value.
        } else {
            return get(); // Return a const pointer to the stored value.
        }
    }

    /// @brief Releases the stored value and resets the storage to its default state.
    /// @return The previously stored value before the storage was reset.
    /// @note This function will move the stored value out of the storage.
    /// For trivial types, it will use default construction to reset the storage.
    constexpr value_type release() noexcept
    {
        if constexpr (std::is_trivial_v<value_type>) {
            return std::exchange(_storage, value_type{}); // Release the current storage and reset it.
        } else {
            return std::move(_storage); // For non-trivial types, just move the storage.
        }
    }

private:
    constexpr static void do_cleanup(value_type* p) noexcept
    {
        if constexpr (std::is_member_function_pointer_v<decltype(Cleanup)>) {
            std::invoke(Cleanup, p);
        } else if constexpr (std::is_invocable_v<decltype(Cleanup), value_type&>) {
            std::invoke(Cleanup, *p);
        } else if constexpr (std::is_invocable_v<decltype(Cleanup), value_type*>) {
            std::invoke(Cleanup, p);
        } else if constexpr (std::is_invocable_v<decltype(Cleanup), value_type>) {
            std::invoke(Cleanup, *p);
        } else {
            static_assert(!sizeof(Cleanup), "Cleanup is not invocable with U&, U*, or U.");
        }
    }

private:
    value_type _storage; // Use storage_type to hold the value.
};
} // namespace vortex