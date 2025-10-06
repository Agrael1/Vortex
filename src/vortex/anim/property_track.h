#pragma once
#include <vortex/anim/keyframe.h>


namespace vortex::anim {
class PropertyTrack
{
public:
    PropertyTrack(std::string_view prop_name,
                  uint32_t prop_index = UINT32_MAX,
                  PropertyType property_type = PropertyType::Void)
        : property_name(prop_name)
        , property_index(prop_index)
        , property_type(property_type)
    {
    }

public: // Track API
    void AddKeyframe(const Keyframe& keyframe) { keyframes.AddKeyframe(keyframe); }
    bool AddKeyframe(std::string_view keyframe_json);
    bool Deserialize(std::string_view track_json);
    void RemoveKeyframe(size_t index) { keyframes.RemoveKeyframe(index); }
    bool HasKeyframes() const { return !keyframes.IsEmpty(); }

    auto GetPropertyIndex() const -> uint32_t { return property_index; }
    auto GetPropertyName() const -> std::string_view { return property_name; }
    auto GetDuration() const->int64_t { return keyframes.GetDuration(); }
    auto GetAbsoluteEndTime() const -> int64_t { return keyframes.GetAbsoluteEndTime(); }

    void SetPreKeyframeBehavior(PreKeyframeBehavior behavior,
                                const PropertyValue& default_val = {});
    void SetPostKeyframeBehavior(PostKeyframeBehavior behavior) { post_behavior = behavior; }


public: // Evaluation
    auto EvaluateAtTime(int64_t time_from_start) const -> PropertyValue;
    void Reset() { keyframes.Clear(); }

private:
    std::string property_name;
    uint32_t property_index = UINT32_MAX;
    PropertyType property_type = PropertyType::Void;
    KeyframeData keyframes;

    PreKeyframeBehavior pre_behavior = PreKeyframeBehavior::Hold;
    PostKeyframeBehavior post_behavior = PostKeyframeBehavior::Hold;
    PropertyValue default_value;
};


// --------------------------------------------------------------
// Inline implementations
inline void PropertyTrack::SetPreKeyframeBehavior(PreKeyframeBehavior behavior,
                                                  const PropertyValue& default_val)
{
    pre_behavior = behavior;
    if (behavior == PreKeyframeBehavior::UseDefault) {
        default_value = default_val;
    }
}
} // namespace vortex::anim