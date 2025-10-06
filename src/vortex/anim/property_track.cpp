#include <vortex/anim/property_track.h>
#include <vortex/util/interp/interpolation.h>

vortex::PropertyValue vortex::anim::PropertyTrack::EvaluateAtTime(int64_t time_from_start) const
{
    if (!HasKeyframes()) {
        return default_value;
    }

    auto [prev_idx, next_idx] = keyframes.FindKeyframeIndices(time_from_start);

    // Before first keyframe
    if (prev_idx == SIZE_MAX) {
        switch (pre_behavior) {
        case PreKeyframeBehavior::Hold:
            return {}; // Keep current value (no change)
        case PreKeyframeBehavior::UseFirstValue:
            return keyframes.GetKeyframe(next_idx).value;
        case PreKeyframeBehavior::UseDefault:
            return default_value;
        }
    }

    // After last keyframe
    if (next_idx == SIZE_MAX) {
        auto last_frame = keyframes.GetKeyframe(prev_idx);
        switch (post_behavior) {
        case PostKeyframeBehavior::Hold:
            return last_frame.value;
        case PostKeyframeBehavior::UseDefault:
            return default_value;
        case PostKeyframeBehavior::Loop:
            // This should be handled at the clip level
            return last_frame.value;
        }
    }

    // Between keyframes - interpolate
    auto prev_frame = keyframes.GetKeyframe(prev_idx);
    auto next_frame = keyframes.GetKeyframe(next_idx);

    int64_t duration = next_frame.time_from_start - prev_frame.time_from_start;
    if (duration <= 0) {
        return prev_frame.value;
    }

    float t = static_cast<float>(time_from_start - prev_frame.time_from_start) / duration;
    t = std::clamp(t, 0.0f, 1.0f);

    // Apply easing
    float eased_t = get_easing_function(prev_frame.ease_type)(t);

    // Interpolate based on value type - use PropertyType bridge system
    return bridge<vortex::interpolate_property_executor>(property_type,
                                                         prev_frame.value,
                                                         next_frame.value,
                                                         eased_t);
}