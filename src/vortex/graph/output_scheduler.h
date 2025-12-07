#pragma once
#include <vortex/sync/pts_clock.h>
#include <vortex/graph/interfaces.h>
#include <vortex/util/rational.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <queue>
#include <chrono>
#include <string>
#include <cstdint>

namespace vortex::graph {

// Round a given PTS to the nearest frame boundary based on framerate
// This is an imprecise boundary rounding, suitable for initial alignment
inline constexpr uint64_t RoundToFrameBoundary(uint64_t pts, vortex::ratio32_t framerate) noexcept
{
    if (framerate.denom() == 0 || framerate.num() == 0) {
        return pts; // Invalid framerate, return original PTS
    }
    uint64_t ticks_per_frame = (sync::PTSClock::timebase_hz * framerate.denom()) / framerate.num();
    if (ticks_per_frame == 0) {
        return pts;
    }
    // Round to nearest frame boundary
    uint64_t frame_number = (pts + ticks_per_frame / 2) / ticks_per_frame;
    return frame_number * ticks_per_frame;
}

// Information about when an output should be evaluated
struct OutputScheduleInfo {
    IOutput* output = nullptr;
    int64_t last_pts = invalid_pts; // When this output was last evaluated
    int64_t base_pts = invalid_pts; // When this output was last evaluated
    int64_t next_pts = invalid_pts; // When this output should be evaluated next
    uint64_t frame_number = 0; // Frame number for this output

public:
    /// @brief Advances to the next frame and updates presentation timestamps.
    /// @return The precious presentation timestamp (PTS) for the current frame, as a int64_t
    /// value.
    constexpr int64_t AdvanceToNextFrame(vortex::ratio32_t framerate) noexcept
    {
        // don't change invalid pts
        if (next_pts == invalid_pts) {
            return invalid_pts;
        }

        ++frame_number;
        last_pts = next_pts;
        next_pts = (sync::PTSClock::timebase_hz * framerate.denom() * frame_number) /
                        framerate.num() +
                base_pts;
        return last_pts;
    }
    constexpr int64_t DurationToNextFrame() const noexcept
    {
        return static_cast<int64_t>(next_pts) - static_cast<int64_t>(last_pts);
    }
    constexpr auto operator<=>(const OutputScheduleInfo& other) const noexcept
    {
        return next_pts <=> other.next_pts;
    }
};

struct OutputSchedulerStats {
    double fps = 0.0;
    uint32_t dropped_frames = 0;
    std::string last_drop_hint;
    uintptr_t last_drop_ptr = 0;
};

// Manages frame-rate aware scheduling of outputs
class OutputScheduler
{
public:
    uint64_t GetCurrentPTS() const noexcept { return _master_clock.CurrentPTS(); }
    void RemoveOutput(IOutput* output) noexcept;
    void AddOutput(IOutput* output) noexcept;
    void Play();
    std::pair<IOutput*, int64_t> GetNextReadyOutput() noexcept;
    OutputSchedulerStats SampleStats() noexcept;

private:
    void UpdateUpperBound(uint64_t pts) noexcept
    {
        if (pts > _upper_boundary_pts) {
            _upper_boundary_pts = pts;
        }
    }

    void RecordPresentedFrame() noexcept;
    void RecordDroppedFrame(const IOutput* output) noexcept;
    void UpdateStatsWindow(bool force = false) noexcept;

private:
    using StatsClock = std::chrono::steady_clock;
    sync::PTSClock _master_clock;
    std::vector<OutputScheduleInfo> _scheduler; // Double-buffered priority queues for scheduling

    uint64_t _upper_boundary_pts = 0; // Last known presentation timestamp from the scheduler

    static constexpr auto kStatsWindow = std::chrono::milliseconds(500);
    StatsClock::time_point _stats_window_start{ StatsClock::now() };
    StatsClock::time_point _last_stats_publish{ StatsClock::now() };
    uint32_t _stats_window_presented = 0;
    uint32_t _stats_window_dropped = 0;
    double _stats_current_fps = 0.0;
    uint32_t _stats_current_dropped = 0;
    std::string _stats_last_drop_hint;
    uintptr_t _stats_last_drop_id = 0;
};

} // namespace vortex::graph