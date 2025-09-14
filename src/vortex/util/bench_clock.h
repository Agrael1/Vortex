#pragma once

#include <chrono>

namespace vortex {
/// @brief A high-resolution clock for benchmarking code execution time.
class bench_clock
{
public:
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;

public:
    /// @brief Gets the current time point.
    /// @return The current time point.
    static time_point now() noexcept
    {
        return clock::now();
    }
    /// @brief Calculates the duration between two time points in milliseconds.
    /// @param start The start time point.
    /// @param end The end time point.
    /// @return The duration in milliseconds.
    static double duration_ms(time_point start, time_point end) noexcept
    {
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    static void print_duration(const char* label, time_point start, time_point end) noexcept
    {
        double ms = duration_ms(start, end);
        vortex::info("{} took {:.3f} ms", label, ms);
    }

    static void print_duration_now(const char* label, time_point start) noexcept
    {
        print_duration(label, start, now());
    }
};
} // namespace vortex
