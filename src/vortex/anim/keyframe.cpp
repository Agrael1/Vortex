#include <vortex/anim/keyframe.h>

// --- KeyframeData methods ---
std::pair<size_t, size_t> vortex::anim::KeyframeData::FindKeyframeIndices(int64_t time) const
{
    if (IsEmpty()) {
        return { SIZE_MAX, SIZE_MAX };
    }

    // Use cached index as starting point for search
    size_t start_search = std::min(_cached_index, Size() - 1);

    // Check if we can use cached result or nearby values
    if (start_search < Size() && time >= _time_from_start[start_search]) {
        // Search forward from cached position
        auto it = std::lower_bound(_time_from_start.begin() + start_search,
                                   _time_from_start.end(),
                                   time);
        size_t index = std::distance(_time_from_start.begin(), it);
        _cached_index = index > 0 ? index - 1 : 0;
    } else {
        // Search from beginning
        auto it = std::lower_bound(_time_from_start.begin(), _time_from_start.end(), time);
        size_t index = std::distance(_time_from_start.begin(), it);
        _cached_index = index > 0 ? index - 1 : 0;
    }

    if (_cached_index >= Size()) {
        _cached_index = Size() - 1;
    }

    // Before first keyframe
    if (time < _time_from_start[0]) {
        return { SIZE_MAX, 0 };
    }

    // After last keyframe
    if (time >= _time_from_start.back()) {
        return { Size() - 1, SIZE_MAX };
    }

    // Between keyframes
    size_t next_index = _cached_index + 1;
    while (next_index < Size() && time >= _time_from_start[next_index]) {
        _cached_index = next_index++;
    }

    return { _cached_index, next_index };
}