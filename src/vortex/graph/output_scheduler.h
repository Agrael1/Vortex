#pragma once
#include <vortex/sync/pts_clock.h>
#include <vortex/graph/interfaces.h>
#include <vortex/util/rational.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <queue>

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
    uint64_t last_pts = 0; // When this output was last evaluated
    uint64_t base_pts = 0; // When this output was last evaluated
    uint64_t next_pts = 0; // When this output should be evaluated next
    uint64_t frame_number = 0; // Frame number for this output

public:
    /// @brief Advances to the next frame and updates presentation timestamps.
    /// @return The precious presentation timestamp (PTS) for the current frame, as a uint64_t
    /// value.
    constexpr uint64_t AdvanceToNextFrame(vortex::ratio32_t framerate) noexcept
    {
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

// Manages frame-rate aware scheduling of outputs
class OutputScheduler
{
public:
    uint64_t GetCurrentPTS() const noexcept { return _master_clock.CurrentPTS(); }
    void RemoveOutput(IOutput* output) noexcept
    {
        // Rebuild the priority queue without the removed output
        auto& current_queue = _scheduler;
        current_queue.erase(std::remove_if(current_queue.begin(),
                                           current_queue.end(),
                                           [output](const OutputScheduleInfo& info) {
                                               return info.output == output;
                                           }),
                            current_queue.end());

        // Re-heapify the priority queue
        std::make_heap(current_queue.begin(), current_queue.end());
    }
    void AddOutput(IOutput* output) noexcept
    {
        // Initialize scheduling info for the new output
        OutputScheduleInfo info;
        info.output = output;
        info.last_pts = 0;
        info.frame_number = 0;
        info.base_pts = RoundToFrameBoundary(_master_clock.CurrentPTS(), output->GetOutputFPS());
        info.next_pts = info.base_pts;

        _scheduler.push_back(info);
        std::push_heap(_scheduler.begin(), _scheduler.end());
    }
    IOutput* GetNextReadyOutput() noexcept
    {
        if (_scheduler.empty()) {
            return nullptr;
        }

        uint64_t current_pts = _master_clock.CurrentPTS();

        if (current_pts > _upper_boundary_pts) {
            // Time has advanced beyond the scheduler's last known PTS
            // Reset all frame counters to avoid drift, and recalculate next_pts
            for (auto& info : _scheduler) {
                info.frame_number = 0;
                info.last_pts = 0;
                info.base_pts = RoundToFrameBoundary(current_pts, info.output->GetOutputFPS());
                info.next_pts = info.base_pts;
            }
            _upper_boundary_pts = current_pts;
        }

        // Check the output at the top of the priority queue
        std::pop_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
        auto& next_info = _scheduler.back();

        // Now there can be three cases:
        // 1. next_info.next_pts < current_pts: The output is overdue, reevaluate pts, advance
        // frame, drop frame, get next output...
        // 2. next_info.next_pts == current_pts +- eps: The output is due now, advance frame, return
        // output
        // 3. next_info.next_pts > current_pts: The output is not due yet, push back and return null

        // Epsilon for timing tolerance (in 90kHz ticks)
        constexpr int64_t epsilon = 200; // ~2.2ms tolerance

        int64_t pts_diff = static_cast<int64_t>(next_info.next_pts) -
                static_cast<int64_t>(current_pts);

        if (pts_diff < -epsilon) {
            // Case 1: Output is overdue, drop frame and advance
            next_info.AdvanceToNextFrame(next_info.output->GetOutputFPS());
            std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
            UpdateUpperBound(next_info.next_pts); // Update upper boundary
            // Report dropped frame (could log or count this event)
            vortex::warn(
                    "OutputScheduler: Dropped frame for output due to being overdue. Output: {}",
                    next_info.output->GetInfo());
            return GetNextReadyOutput(); // Recursively check for the next ready output
        } else if (std::abs(pts_diff) <= epsilon) {
            IOutput* output = next_info.output;
            // Case 2: Output is due now, advance frame and return output
            next_info.AdvanceToNextFrame(next_info.output->GetOutputFPS());
            UpdateUpperBound(next_info.next_pts); // Update upper boundary
            std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
            return output;
        } else {
            // Case 3: Output is not due yet, push back and return null
            std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
            return nullptr;
        }
    }

private:
    void UpdateUpperBound(uint64_t pts) noexcept
    {
        if (pts > _upper_boundary_pts) {
            _upper_boundary_pts = pts;
        }
    }

private:
    sync::PTSClock _master_clock;
    std::vector<OutputScheduleInfo> _scheduler; // Double-buffered priority queues for scheduling

    uint64_t _upper_boundary_pts = 0; // Last known presentation timestamp from the scheduler
};

} // namespace vortex::graph