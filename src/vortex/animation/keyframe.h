#pragma once
#include <vortex/util/interpolation.h>
#include <vortex/properties/type_traits.h>
#include <vector>
#include <any>
#include <string>
#include <chrono>
#include <algorithm>

namespace vortex::animation {

// Represents a single keyframe for a property
struct Keyframe {
    float time; // Time in seconds
    std::any value; // The value at this time
    EaseType ease_type = EaseType::Linear; // Easing to use when interpolating TO this keyframe
};

// Represents an animation track for a single property
class PropertyTrack {
public:
    PropertyTrack(std::string property_name, PropertyType type)
        : property_name_(std::move(property_name)), property_type_(type) {}
    
    // Add a keyframe to this track
    template<typename T>
    void AddKeyframe(float time, const T& value, EaseType ease = EaseType::Linear) {
        keyframes_.emplace_back(time, value, ease);
        std::sort(keyframes_.begin(), keyframes_.end(), 
                  [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }
    
    void AddKeyframe(const Keyframe& keyframe) {
        keyframes_.push_back(keyframe);
        std::sort(keyframes_.begin(), keyframes_.end(), 
                  [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }
    
    // Remove keyframe at specific time
    void RemoveKeyframe(float time, float tolerance = 0.001f) {
        keyframes_.erase(
            std::remove_if(keyframes_.begin(), keyframes_.end(),
                          [time, tolerance](const Keyframe& kf) {
                              return std::abs(kf.time - time) < tolerance;
                          }),
            keyframes_.end());
    }
    
    // Get interpolated value at time
    std::any GetValue(float time) const {
        if (keyframes_.empty()) {
            return std::any{};
        }
        
        if (keyframes_.size() == 1 || time <= keyframes_.front().time) {
            return keyframes_.front().value;
        }
        
        if (time >= keyframes_.back().time) {
            return keyframes_.back().value;
        }
        
        // Find the keyframes to interpolate between
        auto it = std::lower_bound(keyframes_.begin(), keyframes_.end(), time,
                                  [](const Keyframe& kf, float t) { return kf.time < t; });
        
        if (it == keyframes_.end()) {
            return keyframes_.back().value;
        }
        
        auto next_kf = *it;
        auto prev_kf = *(--it);
        
        // Calculate interpolation factor
        float duration = next_kf.time - prev_kf.time;
        if (duration <= 0.0f) {
            return prev_kf.value;
        }
        
        float t = (time - prev_kf.time) / duration;
        
        // Use the RuntimePropertyInterpolator for type-erased interpolation
        RuntimePropertyInterpolator interpolator(property_type_, prev_kf.value, next_kf.value, next_kf.ease_type);
        return interpolator.interpolate(t);
    }
    
    // Get all keyframes (read-only)
    const std::vector<Keyframe>& GetKeyframes() const { return keyframes_; }
    
    // Get property info
    const std::string& GetPropertyName() const { return property_name_; }
    PropertyType GetPropertyType() const { return property_type_; }
    
    // Get the duration of this track
    float GetDuration() const {
        return keyframes_.empty() ? 0.0f : keyframes_.back().time;
    }
    
    // Check if track has keyframes
    bool IsEmpty() const { return keyframes_.empty(); }
    
    // Clear all keyframes
    void Clear() { keyframes_.clear(); }
    
private:
    std::string property_name_;
    PropertyType property_type_;
    std::vector<Keyframe> keyframes_;
};

// Animation clip containing multiple property tracks
class AnimationClip {
public:
    AnimationClip(std::string name = "Animation") 
        : name_(std::move(name)), duration_(0.0f), loop_(false) {}
    
    // Add a property track
    PropertyTrack& AddTrack(const std::string& property_name, PropertyType type) {
        tracks_[property_name] = std::make_unique<PropertyTrack>(property_name, type);
        return *tracks_[property_name];
    }
    
    // Get a property track
    PropertyTrack* GetTrack(const std::string& property_name) {
        auto it = tracks_.find(property_name);
        return it != tracks_.end() ? it->second.get() : nullptr;
    }
    
    const PropertyTrack* GetTrack(const std::string& property_name) const {
        auto it = tracks_.find(property_name);
        return it != tracks_.end() ? it->second.get() : nullptr;
    }
    
    // Remove a property track
    void RemoveTrack(const std::string& property_name) {
        tracks_.erase(property_name);
        RecalculateDuration();
    }
    
    // Get all track names
    std::vector<std::string> GetTrackNames() const {
        std::vector<std::string> names;
        names.reserve(tracks_.size());
        for (const auto& [name, track] : tracks_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Sample all properties at given time
    std::unordered_map<std::string, std::any> Sample(float time) const {
        std::unordered_map<std::string, std::any> result;
        
        // Handle looping
        float sample_time = time;
        if (loop_ && duration_ > 0.0f) {
            sample_time = std::fmod(time, duration_);
            if (sample_time < 0.0f) {
                sample_time += duration_;
            }
        }
        
        for (const auto& [name, track] : tracks_) {
            if (!track->IsEmpty()) {
                result[name] = track->GetValue(sample_time);
            }
        }
        
        return result;
    }
    
    // Animation properties
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    
    float GetDuration() const { return duration_; }
    void SetDuration(float duration) { duration_ = duration; }
    
    bool IsLooping() const { return loop_; }
    void SetLooping(bool loop) { loop_ = loop; }
    
    // Recalculate duration based on tracks
    void RecalculateDuration() {
        duration_ = 0.0f;
        for (const auto& [name, track] : tracks_) {
            duration_ = std::max(duration_, track->GetDuration());
        }
    }
    
    // Clear all tracks
    void Clear() {
        tracks_.clear();
        duration_ = 0.0f;
    }
    
    bool IsEmpty() const { return tracks_.empty(); }
    
private:
    std::string name_;
    float duration_;
    bool loop_;
    std::unordered_map<std::string, std::unique_ptr<PropertyTrack>> tracks_;
};

// Animation state for playback
class AnimationState {
public:
    enum class PlayState {
        Stopped,
        Playing,
        Paused
    };
    
    AnimationState() 
        : current_time_(0.0f), playback_speed_(1.0f), state_(PlayState::Stopped) {}
    
    void Play() { 
        state_ = PlayState::Playing; 
        last_update_ = std::chrono::steady_clock::now();
    }
    
    void Pause() { 
        state_ = PlayState::Paused; 
    }
    
    void Stop() { 
        state_ = PlayState::Stopped; 
        current_time_ = 0.0f;
    }
    
    void SetTime(float time) { 
        current_time_ = time; 
        last_update_ = std::chrono::steady_clock::now();
    }
    
    void Update() {
        if (state_ != PlayState::Playing) {
            return;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration<float>(now - last_update_).count();
        last_update_ = now;
        
        current_time_ += delta * playback_speed_;
    }
    
    float GetCurrentTime() const { return current_time_; }
    float GetPlaybackSpeed() const { return playback_speed_; }
    void SetPlaybackSpeed(float speed) { playback_speed_ = speed; }
    
    PlayState GetState() const { return state_; }
    
private:
    float current_time_;
    float playback_speed_;
    PlayState state_;
    std::chrono::steady_clock::time_point last_update_;
};

} // namespace vortex::animation