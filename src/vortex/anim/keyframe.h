#pragma once
#include <vortex/properties/type_traits.h>
#include <vortex/util/interp/easing.h>

namespace vortex::anim {
enum class PreKeyframeBehavior {
    Hold, // Keep property unchanged
    UseFirstValue, // Assume first keyframe value
    UseDefault // Use specified default
};

enum class PostKeyframeBehavior {
    Hold, // Keep last keyframe value
    UseDefault, // Return to default
    Loop // Loop the animation
};

enum class LoopMode {
    None, // Play once
    Repeat, // Loop from start
    PingPong, // Bounce back and forth
    Hold // Hold at end
};

struct Keyframe {
    int64_t time_from_start;
    PropertyValue value;
    EaseType ease_type = EaseType::Linear;
};

class KeyframeData
{
public:
    void Clear();
    bool IsEmpty() const { return _time_from_start.empty(); }
    auto Size() const -> size_t { return _time_from_start.size(); }

    void AddKeyframe(const Keyframe& frame);
    void RemoveKeyframe(size_t index);

    // Optimized keyframe lookup with caching
    auto FindKeyframeIndices(int64_t time) const -> std::pair<size_t, size_t>;
    auto GetKeyframe(size_t index) const -> Keyframe;
    auto GetDuration() const -> int64_t;
    auto GetAbsoluteEndTime() const -> int64_t;

private:
    std::vector<int64_t> _time_from_start;
    std::vector<PropertyValue> _value;
    std::vector<EaseType> _ease_type;
    mutable size_t _cached_index = 0; // Cache for optimization
};

// --------------------------------------------------------------
// Inlne implementations
inline void KeyframeData::Clear()
{
    _time_from_start.clear();
    _value.clear();
    _ease_type.clear();
    _cached_index = 0;
}

inline void KeyframeData::AddKeyframe(const Keyframe& frame)
{
    // Insert while maintaining order
    auto it = std::lower_bound(_time_from_start.begin(),
                               _time_from_start.end(),
                               frame.time_from_start);
    size_t index = std::distance(_time_from_start.begin(), it);
    _time_from_start.insert(it, frame.time_from_start);
    _value.insert(_value.begin() + index, frame.value);
    _ease_type.insert(_ease_type.begin() + index, frame.ease_type);
    _cached_index = 0; // Reset cache
}

inline void KeyframeData::RemoveKeyframe(size_t index)
{
    if (index < Size()) {
        _time_from_start.erase(_time_from_start.begin() + index);
        _value.erase(_value.begin() + index);
        _ease_type.erase(_ease_type.begin() + index);
        _cached_index = 0; // Reset cache
    }
}

inline Keyframe KeyframeData::GetKeyframe(size_t index) const
{
    if (index >= Size()) {
        return {};
    }
    return { _time_from_start[index], _value[index], _ease_type[index] };
}

inline int64_t KeyframeData::GetDuration() const
{
    return IsEmpty() ? 0 : _time_from_start.back() - _time_from_start.front();
}

inline int64_t KeyframeData::GetAbsoluteEndTime() const
{
    return IsEmpty() ? 0 : _time_from_start.back();
}
} // namespace vortex::anim