#pragma once
#include <vortex/anim/property_track.h>

namespace vortex::graph {
struct INode;
} // namespace vortex::graph

namespace vortex::anim {

enum class AnimationState {
    Playing,
    Paused,
    Stopped
};

class AnimationClip
{
public:
    AnimationClip(vortex::graph::INode* target_node)
        : _target_node(target_node)
    {
    }

public:
    auto AddPropertyTrack(std::string_view property_name) -> PropertyTrack*;
    void RemovePropertyTrack(PropertyTrack* track);
    auto GetTracks() const -> std::span<const std::unique_ptr<PropertyTrack>> { return _tracks; }

    void SetLoopMode(LoopMode mode) { _loop_mode = mode; }
    void SetDuration(int64_t duration_pts) { _clip_duration = duration_pts; }
    void SetStartTime(int64_t start_pts) { _start_time = start_pts; }

    auto GetLoopMode() const -> LoopMode { return _loop_mode; }
    auto GetDuration() const -> int64_t { return _clip_duration; }
    auto GetStartTime() const -> int64_t { return _start_time; }
    auto GetState() const -> AnimationState { return _state; }

    // Animation control
    void Play(int64_t current_pts = -1);
    void Pause(int64_t current_pts);
    void Stop();
    void Resume(int64_t current_pts);

    // Calculate effective duration including all tracks
    auto CalculateEffectiveDuration() const -> int64_t;

    // Evaluate all tracks at given time and apply to node
    void EvaluateAtTime(int64_t global_pts);

    // Transform global PTS to local time with loop handling (public for external access)
    auto GetLocalTime(int64_t global_pts) const -> int64_t;

private:
    // Apply loop transformation to time
    auto ApplyLoopTransform(int64_t local_time, int64_t duration) const -> int64_t;

private:
    vortex::graph::INode* _target_node = nullptr;
    std::vector<std::unique_ptr<PropertyTrack>> _tracks;

    LoopMode _loop_mode = LoopMode::None;
    int64_t _clip_duration = 0;
    int64_t _start_time = 0;
    
    // Animation state management
    AnimationState _state = AnimationState::Stopped;
    int64_t _pause_time = 0;          // PTS when animation was paused
    int64_t _pause_local_time = 0;    // Local animation time when paused
    int64_t _accumulated_pause_time = 0; // Total time spent paused
};


inline void AnimationClip::RemovePropertyTrack(PropertyTrack* track) {
    std::erase_if(_tracks, [track](const auto& ptr) { return ptr.get() == track; });
}
} // namespace vortex::anim