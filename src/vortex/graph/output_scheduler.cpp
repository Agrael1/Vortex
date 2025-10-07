#include <vortex/graph/output_scheduler.h>
#include <vortex/util/log.h>

void vortex::graph::OutputScheduler::RemoveOutput(IOutput* output) noexcept
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
    std::make_heap(current_queue.begin(), current_queue.end(), std::greater<>{});
}

void vortex::graph::OutputScheduler::AddOutput(IOutput* output) noexcept
{
    // Initialize scheduling info for the new output
    OutputScheduleInfo info;
    info.output = output;
    info.last_pts = 0;
    info.frame_number = 0;
    info.base_pts = RoundToFrameBoundary(_master_clock.CurrentPTS(), output->GetOutputFPS());
    info.next_pts = info.base_pts;

    _scheduler.push_back(info);
    std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
}

void vortex::graph::OutputScheduler::Play()
{
    // Reset the master clock and scheduling info for all outputs
    auto now = _master_clock.CurrentPTS();

    _master_clock.Reset();
    for (auto& info : _scheduler) {
        info.frame_number = 0;
        info.last_pts = 0;
        info.base_pts = now;
        info.next_pts = now;
    }

    // Reset base PTS for all outputs
    for (auto& info : _scheduler) {
        info.output->SetBasePTS(now);
    }
}

std::pair<vortex::graph::IOutput*, int64_t>
vortex::graph::OutputScheduler::GetNextReadyOutput() noexcept
{
    if (_scheduler.empty()) {
        return {nullptr, invalid_pts};
    }

    uint64_t current_pts = _master_clock.CurrentPTS();

    if (current_pts > _upper_boundary_pts) {
        // Time has advanced beyond the scheduler's last known PTS
        // Reset all frame counters to avoid drift, and recalculate next_pts
        for (auto& info : _scheduler) {
            info.frame_number = 0;
            info.last_pts = 0;
            info.base_pts = current_pts;
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

    int64_t pts_diff = static_cast<int64_t>(next_info.next_pts) - static_cast<int64_t>(current_pts);

    if (pts_diff < -epsilon) {
        // Case 1: Output is overdue, drop frame and advance
        next_info.AdvanceToNextFrame(next_info.output->GetOutputFPS());
        std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
        UpdateUpperBound(next_info.next_pts); // Update upper boundary
        // Report dropped frame (could log or count this event)
        vortex::warn("OutputScheduler: Dropped frame for output due to being overdue. Output: {}",
                     next_info.output->GetInfo());
        return GetNextReadyOutput(); // Recursively check for the next ready output
    } else if (std::abs(pts_diff) <= epsilon) {
        IOutput* output = next_info.output;
        // Case 2: Output is due now, advance frame and return output
        auto present_pts = next_info.AdvanceToNextFrame(next_info.output->GetOutputFPS());
        UpdateUpperBound(next_info.next_pts); // Update upper boundary
        std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
        return { output, present_pts }; // Return the output and its presentation timestamp
    } else {
        // Case 3: Output is not due yet, push back and return null
        std::push_heap(_scheduler.begin(), _scheduler.end(), std::greater<>{});
        return { nullptr, invalid_pts };
    }
}
