#include <vortex/anim/animation_clip.h>
#include <vortex/graph/interfaces.h>

auto vortex::anim::AnimationClip::AddPropertyTrack(std::string_view property_name) -> PropertyTrack*
{
    auto [prop_index, type] = _target_node->GetPropertyDesc(property_name);
    auto it = std::find_if(_tracks.begin(), _tracks.end(), [=](const auto& existing) {
        return existing->GetPropertyIndex() == prop_index;
    });

    PropertyTrack* track = nullptr;
    if (it != _tracks.end()) {
        (*it)->Reset(); // Clear existing keyframes
        track = it->get();
    } else {
        // Create new track
        if (prop_index == invalid_property_index) {
            vortex::error("Property '{}' not found on target node", property_name);
            return track;
        }
        auto new_track = std::make_unique<PropertyTrack>(property_name, prop_index, type);
        track = new_track.get();
        _tracks.push_back(std::move(new_track));
    }
    return track;
}

void vortex::anim::AnimationClip::Play(int64_t current_pts)
{
    if (current_pts != -1) {
        _start_time = current_pts;
    }
    _state = AnimationState::Playing;
    _accumulated_pause_time = 0;
    _pause_time = 0;
    _pause_local_time = 0;
}

void vortex::anim::AnimationClip::Pause(int64_t current_pts)
{
    if (_state != AnimationState::Playing) {
        return;
    }
    
    _state = AnimationState::Paused;
    _pause_time = current_pts;
    _pause_local_time = GetLocalTime(current_pts);
}

void vortex::anim::AnimationClip::Resume(int64_t current_pts)
{
    if (_state != AnimationState::Paused) {
        return;
    }
    
    _state = AnimationState::Playing;
    
    // Calculate how long we were paused and adjust start time accordingly
    int64_t pause_duration = current_pts - _pause_time;
    _accumulated_pause_time += pause_duration;
    
    // Adjust start time to maintain animation position
    _start_time += pause_duration;
}

void vortex::anim::AnimationClip::Stop()
{
    _state = AnimationState::Stopped;
    _accumulated_pause_time = 0;
    _pause_time = 0;
    _pause_local_time = 0;
}

int64_t vortex::anim::AnimationClip::CalculateEffectiveDuration() const
{
    if (_clip_duration > 0) {
        return _clip_duration;
    }

    int64_t max_end_time = 0;
    for (const auto& track : _tracks) {
        if (!track->HasKeyframes()) {
            continue;
        }
        
        // Get the absolute end time of the track (start_time + duration)
        int64_t track_end_time = track->GetAbsoluteEndTime();
        max_end_time = std::max(max_end_time, track_end_time);
    }

    return max_end_time;
}

int64_t vortex::anim::AnimationClip::GetLocalTime(int64_t global_pts) const
{
    if (_state == AnimationState::Stopped) {
        return -1;
    }
    
    if (_state == AnimationState::Paused) {
        return _pause_local_time;
    }
    
    if (global_pts < _start_time) {
        return -1; // Not started yet
    }

    int64_t local_time = global_pts - _start_time;
    int64_t duration = CalculateEffectiveDuration();

    if (duration <= 0) {
        return local_time;
    }

    return ApplyLoopTransform(local_time, duration);
}

void vortex::anim::AnimationClip::EvaluateAtTime(int64_t global_pts)
{
    if (_state == AnimationState::Stopped) {
        return;
    }
    
    int64_t local_time = GetLocalTime(global_pts);
    if (local_time < 0) {
        return; // Animation hasn't started
    }

    for (const auto& track : _tracks) {
        if (!track->HasKeyframes()) {
            continue;
        }

        PropertyValue value = track->EvaluateAtTime(local_time);
        if (std::holds_alternative<std::monostate>(value)) {
            continue; // No value to apply
        }

        // Apply to node using property index
        uint32_t prop_index = track->GetPropertyIndex();
        if (prop_index != invalid_property_index) {
            _target_node->SetProperty(prop_index, value, false); // Don't notify during animation
        }
    }
}

int64_t vortex::anim::AnimationClip::ApplyLoopTransform(int64_t local_time, int64_t duration) const
{
    if (duration <= 0 || local_time < 0) {
        return local_time;
    }

    switch (_loop_mode) {
    case LoopMode::None:
    case LoopMode::Hold:
        return std::min(local_time, duration);

    case LoopMode::Repeat:
        return local_time % duration;

    case LoopMode::PingPong: {
        int64_t cycle_length = duration * 2;
        int64_t cycle_time = local_time % cycle_length;

        if (cycle_time < duration) {
            return cycle_time;
        } else {
            return duration - (cycle_time - duration);
        }
    }
    }

    return local_time;
}