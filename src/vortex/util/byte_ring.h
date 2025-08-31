#pragma once
#include <memory>
#include <cstring>
#include <system_error>
#include <span>

namespace vortex {
class byte_ring
{
public:
    constexpr byte_ring() noexcept = default;
    constexpr explicit byte_ring(size_t initial_capacity) noexcept
        : _capacity(initial_capacity)
    {
        if (_capacity > 0) {
            _data.reset(new (std::nothrow) std::byte[_capacity]);
            if (!_data) {
                _capacity = 0;
            }
        }
    }
    constexpr byte_ring(byte_ring&& other) noexcept
        : _data(std::move(other._data))
        , _head(other._head)
        , _tail(other._tail)
        , _capacity(other._capacity)
    {
        other._head = 0;
        other._tail = 0;
        other._capacity = 0;
    }
    constexpr byte_ring& operator=(byte_ring&& other) noexcept
    {
        if (this != &other) {
            _data = std::move(other._data);
            _head = other._head;
            _tail = other._tail;
            _capacity = other._capacity;

            other._head = 0;
            other._tail = 0;
            other._capacity = 0;
        }
        return *this;
    }

public:
    [[nodiscard]] std::errc write(std::span<const std::byte> data) noexcept
    {
        if (data.empty()) {
            return {};
        }

        auto result = expand_if_needed(data.size());
        if (result != std::errc{}) {
            return result;
        }

        // Write data in one or two chunks depending on wrap-around
        size_t first_chunk = std::min(data.size(), _capacity - _tail);
        std::memcpy(_data.get() + _tail, data.data(), first_chunk);

        if (data.size() > first_chunk) {
            // Wrap around to the beginning
            std::memcpy(_data.get(), data.data() + first_chunk, data.size() - first_chunk);
        }

        _tail = (_tail + data.size()) % _capacity;
        return {};
    }
    [[nodiscard]] std::size_t read(std::span<std::byte> buffer) noexcept
    {
        size_t available = this->size();
        size_t to_read = std::min(buffer.size(), available);

        if (to_read == 0) {
            return 0;
        }

        // Read data in one or two chunks depending on wrap-around
        size_t first_chunk = std::min(to_read, _capacity - _head);
        std::memcpy(buffer.data(), _data.get() + _head, first_chunk);

        if (to_read > first_chunk) {
            // Wrap around to the beginning
            std::memcpy(buffer.data() + first_chunk, _data.get(), to_read - first_chunk);
        }

        _head = (_head + to_read) % _capacity;
        return to_read;
    }
    [[nodiscard]] std::size_t peek(std::span<std::byte> buffer, size_t offset = 0) const noexcept
    {
        size_t available = size();
        if (offset >= available) {
            return 0;
        }

        size_t to_peek = std::min(buffer.size(), available - offset);
        if (to_peek == 0) {
            return 0;
        }

        size_t peek_start = (_head + offset) % _capacity;

        // Peek data in one or two chunks depending on wrap-around
        size_t first_chunk = std::min(to_peek, _capacity - peek_start);
        std::memcpy(buffer.data(), _data.get() + peek_start, first_chunk);

        if (to_peek > first_chunk) {
            // Wrap around to the beginning
            std::memcpy(buffer.data() + first_chunk, _data.get(), to_peek - first_chunk);
        }

        return to_peek;
    }

    template<typename T, size_t extent>
    [[nodiscard]] std::errc peek_as(std::span<T, extent> data, size_t offset_bytes = 0) noexcept
    {
        return peek(std::as_writable_bytes(data), offset_bytes);
    }
    template<typename T, size_t extent>
    [[nodiscard]] std::errc write_as(std::span<T, extent> data) noexcept
    {
        return write(std::as_bytes(data));
    }
    template<typename T, size_t extent>
    [[nodiscard]] std::size_t read_as(std::span<T, extent> data) noexcept
    {
        return read(std::as_writable_bytes(data)) / sizeof(T);
    }

    constexpr void skip(std::size_t bytes) noexcept
    {
        std::size_t to_skip = std::min(bytes, size());
        _head = (_head + to_skip) % _capacity;
    }

    constexpr void clear() noexcept
    {
        _head = 0;
        _tail = 0;
    }

    [[nodiscard]] constexpr std::errc reserve(std::size_t new_capacity) noexcept
    {
        if (new_capacity > _capacity) {
            return expand_to_capacity(new_capacity);
        }
        return {};
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        if (_capacity == 0) {
            return 0;
        }
        return (_tail >= _head)
                ? (_tail - _head)
                : (_capacity - _head + _tail);
    }
    [[nodiscard]] constexpr std::size_t capacity() const noexcept
    {
        return _capacity;
    }
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _head == _tail;
    }
    [[nodiscard]] constexpr std::size_t available_space() const noexcept
    {
        if (_capacity == 0) {
            return 0;
        }
        return _capacity - size() - 1; // -1 to distinguish between full and empty
    }

private:
    // Helper methods
    [[nodiscard]] constexpr std::errc expand_if_needed(std::size_t required_space) noexcept
    {
        size_t current_available = available_space();

        if (required_space > current_available) {
            size_t new_capacity = std::max(_capacity * 2, std::bit_ceil(size() + required_space + 1));
            return expand_to_capacity(new_capacity);
        }
        return {};
    }
    [[nodiscard]] constexpr std::errc expand_to_capacity(std::size_t new_capacity) noexcept
    {
        if (new_capacity <= _capacity) {
            return {};
        }

        auto new_data = std::unique_ptr<std::byte[]>(new (std::nothrow) std::byte[new_capacity]);
        if (!new_data) {
            return std::errc::not_enough_memory;
        }

        if (_capacity > 0) {
            copy_data_to_new_buffer(new_data.get(), new_capacity);
        }

        _data = std::move(new_data);
        _capacity = new_capacity;
        return {};
    }
    constexpr void copy_data_to_new_buffer(std::byte* new_data, std::size_t new_capacity) noexcept
    {
        size_t current_size = size();
        if (current_size == 0) {
            return;
        }

        // Copy existing data to the beginning of the new buffer
        if (_head <= _tail) {
            // Data is contiguous
            std::memcpy(new_data, _data.get() + _head, current_size);
        } else {
            // Data wraps around
            size_t first_chunk = _capacity - _head;
            std::memcpy(new_data, _data.get() + _head, first_chunk);
            std::memcpy(new_data + first_chunk, _data.get(), _tail);
        }

        // Reset head and tail for the linearized data
        _head = 0;
        _tail = current_size;
    }

private:
    std::unique_ptr<std::byte[]> _data;
    std::size_t _head = 0;
    std::size_t _tail = 0;
    std::size_t _capacity = 0;
};
} // namespace vortex
