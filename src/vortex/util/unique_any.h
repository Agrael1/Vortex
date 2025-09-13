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

/// @brief Strategy for clearing/resetting storage in unique_any
enum class clear_strategy {
    /// @brief Clear the current storage before putting a new value
    /// @details Only calls the cleanup function without reconstructing the storage
    none, 
    /// @brief Reset the storage with default construction before putting a new value  
    /// @details Calls cleanup function and then default-constructs the storage
    default_construct 
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
    constexpr unique_any() noexcept
        requires std::is_default_constructible_v<value_type>
        : _storage{}
    {
    }
    constexpr unique_any(std::nullptr_t) noexcept
        requires detail::is_handle_or_pointer_v<value_type>
        : _storage{ nullptr }
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
        if (this != std::addressof(other)) {
            reset(); // Cleanup the current object before moving.
            _storage = std::exchange(other._storage, value_type{}); // Move the storage from the other object.
        }
        return *this;
    }
    constexpr ~unique_any() noexcept
    {
        clear<clear_strategy::none>(); // No need to reset, just cleanup the storage.
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

    /// @brief Clears storage and returns pointer for external initialization.
    /// @tparam Strategy Controls post-cleanup behavior (default: default_construct for safety).
    /// @return Pointer to storage after cleanup and optional reconstruction.
    /// @note Use default_construct for guaranteed clean state (nullptr/0/default-initialized).
    /// @note Use none for performance when external code will immediately overwrite storage.
    /// @warning With Strategy::none, storage may be in an undefined state until overwritten.
    template<clear_strategy Strategy = clear_strategy::default_construct>
    constexpr value_type* put() noexcept
    {
        clear<Strategy>(); // Clear the current storage before returning the pointer.
        return &_storage; // Return a pointer to the storage.
    }

    /// @brief Performs cleanup and optionally reconstructs the stored value.
    /// @tparam Strategy Controls behavior after cleanup (default: default_construct).
    /// @note Strategy::None - Only calls cleanup function. Storage remains in post-cleanup state.
    /// @note Strategy::DefaultConstruct - Calls cleanup, then destroys and default-constructs fresh value.
    /// @warning With Strategy::none, is_valid() may still return true if cleanup doesn't invalidate the value.
    /// @warning With Strategy::none, accessing the value after clear() may be undefined behavior.
    template<clear_strategy Strategy = clear_strategy::default_construct>
    constexpr void clear() noexcept
    {
        if (traits_type::is_valid(_storage)) {
            do_cleanup(&_storage); // Cleanup the current storage.
        }

        if constexpr (Strategy == clear_strategy::default_construct) {
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                std::destroy_at(&_storage); // Clean up the current storage.
            }
            std::construct_at(&_storage); // Default construct the storage.
        }
    }

    /// @brief Resets the storage with new arguments.
    /// @param ...args Arguments to construct the new value in storage.
    template<typename... Args>
    constexpr void reset(Args&&... args) noexcept
    {
        clear<clear_strategy::none>(); // Cleanup the current storage.

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

    constexpr value_type* address_of() noexcept
    {
        return std::addressof(_storage); // Return a pointer to the storage.
    }
    constexpr const value_type* address_of() const noexcept
    {
        return std::addressof(_storage); // Return a const pointer to the storage.
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