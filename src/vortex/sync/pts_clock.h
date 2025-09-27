#pragma once
#include <vortex/util/rational.h>
#include <vortex/sync/wall_clock.h>
#include <chrono>
#include <cstdint>

namespace vortex::sync {

// PTS (Presentation Time Stamp) utilities using 90kHz timebase (standard for video)
class PTSClock
{
public:
    // 90kHz timebase is standard for video timing (used in MPEG, etc.)
    static constexpr uint64_t timebase_hz = 90000;
    static constexpr vortex::ratio32_t timebase = { 1, static_cast<int32_t>(timebase_hz) };

public:
    // Reset the clock to start counting from zero
    void Reset() noexcept
    {
        _wall_clock.Reset();
        _start_pts = 0;
    }

    // Get current PTS based on wall clock elapsed time
    uint64_t CurrentPTS() const noexcept
    {
        auto elapsed_ns = _wall_clock.ElapsedNanoseconds();
        // Convert nanoseconds to 90kHz ticks
        return static_cast<uint64_t>((elapsed_ns * timebase_hz) / 1'000'000'000);
    }

    // Calculate PTS for the next frame based on framerate
    uint64_t NextFramePTS(uint64_t current_pts, vortex::ratio32_t framerate) const noexcept
    {
        if (framerate.denom() == 0 || framerate.num() == 0) {
            return current_pts; // Invalid framerate, return current PTS
        }

        // Calculate ticks per frame in 90kHz timebase
        uint64_t ticks_per_frame = (timebase_hz * framerate.denom()) / framerate.num();
        return current_pts + ticks_per_frame;
    }

    // Calculate the wall clock time for a given PTS
    std::chrono::nanoseconds PTSToWallTime(uint64_t pts) const noexcept
    {
        // Convert 90kHz ticks to nanoseconds
        uint64_t ns = (pts * 1'000'000'000) / timebase_hz;
        return std::chrono::nanoseconds(ns);
    }

    // Calculate how many nanoseconds to wait for a target PTS
    std::chrono::nanoseconds TimeUntilPTS(uint64_t target_pts) const noexcept
    {
        uint64_t current_pts = CurrentPTS();
        if (target_pts <= current_pts) {
            return std::chrono::nanoseconds(0); // Already past target time
        }

        uint64_t pts_diff = target_pts - current_pts;
        uint64_t ns_diff = (pts_diff * 1'000'000'000) / timebase_hz;
        return std::chrono::nanoseconds(ns_diff);
    }

    // Round PTS to the nearest frame boundary for a given framerate
    uint64_t RoundToFrameBoundary(uint64_t pts, vortex::ratio32_t framerate) const noexcept
    {
        if (framerate.denom() == 0 || framerate.num() == 0) {
            return pts; // Invalid framerate, return original PTS
        }

        uint64_t ticks_per_frame = (timebase_hz * framerate.denom()) / framerate.num();
        if (ticks_per_frame == 0) {
            return pts;
        }

        // Round to nearest frame boundary
        uint64_t frame_number = (pts + ticks_per_frame / 2) / ticks_per_frame;
        return frame_number * ticks_per_frame;
    }

private:
    WallClock _wall_clock;
    uint64_t _start_pts = 0;
};

} // namespace vortex::sync