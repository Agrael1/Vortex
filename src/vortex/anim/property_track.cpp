#include <vortex/anim/property_track.h>
#include <vortex/util/interp/interpolation.h>
#include <vortex/util/deserialize.h>
#include <nlohmann/json.hpp>

bool vortex::anim::PropertyTrack::AddKeyframe(std::string_view keyframe_json)
{
    // Parse JSON and add keyframe
    try {
        auto json = nlohmann::json::parse(keyframe_json);
        Keyframe frame;
        frame.time_from_start = json.at("time_from_start").get<int64_t>();
        frame.value = bridge<deserialize_executor>(property_type, json.at("value").dump());
        if (json.contains("ease_type")) {
            frame.ease_type = json.at("ease_type").get<EaseType>();
        }
        AddKeyframe(frame);
    } catch (const std::exception& e) {
        vortex::error("Failed to parse keyframe JSON: {}", e.what());
        return false;
    }
    return true;
}

bool vortex::anim::PropertyTrack::Deserialize(std::string_view track_json)
{
    try {
        auto json = nlohmann::json::parse(track_json);
        if (json.contains("property_name")) {
            property_name = json.at("property_name").get<std::string>();
        }
        if (json.contains("property_index")) {
            property_index = json.at("property_index").get<uint32_t>();
        }
        if (json.contains("property_type")) {
            property_type = json.at("property_type").get<PropertyType>();
        }
        if (json.contains("pre_behavior")) {
            pre_behavior = json.at("pre_behavior").get<PreKeyframeBehavior>();
        }
        if (json.contains("post_behavior")) {
            post_behavior = json.at("post_behavior").get<PostKeyframeBehavior>();
        }
        if (json.contains("default_value")) {
            default_value = bridge<deserialize_executor>(property_type, json.at("default_value").dump());
        }
        if (json.contains("keyframes") && json.at("keyframes").is_array()) {
            for (const auto& kf_json : json.at("keyframes")) {
                Keyframe frame;
                frame.time_from_start = kf_json.at("time_from_start").get<int64_t>();
                frame.value = bridge<deserialize_executor>(property_type, kf_json.at("value").dump());
                if (kf_json.contains("ease_type")) {
                    frame.ease_type = kf_json.at("ease_type").get<EaseType>();
                }
                AddKeyframe(frame);
            }
        }
    } catch (const std::exception& e) {
        vortex::error("Failed to parse track JSON: {}", e.what());
        return false;
    }
    return true;
}

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