#pragma once
#include <chrono>
#include <vortex/util/rational.h>

namespace vortex::sync {
class WallClock
{
public:
    static constexpr vortex::ratio32_t resolution = vortex::ratio32_t{ std::chrono::steady_clock::period::num, std::chrono::steady_clock::period::den };

public:
    WallClock() = default;
    WallClock(const WallClock&) = delete;
    WallClock& operator=(const WallClock&) = delete;
    WallClock(WallClock&&) = default;
    WallClock& operator=(WallClock&&) = default;

public:
    void Reset() noexcept
    {
        start_time = std::chrono::steady_clock::now();
    }
    std::chrono::steady_clock::duration Elapsed() const noexcept
    {
        return std::chrono::steady_clock::now() - start_time;
    }
    int64_t ElapsedNanoseconds() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed()).count();
    }

private:
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
};
} // namespace vortex::sync