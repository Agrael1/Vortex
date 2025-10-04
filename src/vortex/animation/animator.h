#pragma once
#include <vortex/animation/keyframe.h>
#include <vortex/properties/props.hpp>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace vortex::animation {

// Interface for objects that can be animated
template<typename PropertiesType>
class AnimationTarget {
public:
    virtual ~AnimationTarget() = default;
    
    // Apply animated property values
    virtual void ApplyAnimatedProperties(const std::unordered_map<std::string, std::any>& properties) = 0;
    
    // Get property type information
    virtual PropertyType GetPropertyType(const std::string& property_name) const = 0;
    
    // Get current property value as std::any
    virtual std::any GetPropertyValue(const std::string& property_name) const = 0;
};

// Default implementation for generated property types
template<typename PropertiesType>
class DefaultAnimationTarget : public AnimationTarget<PropertiesType> {
public:
    DefaultAnimationTarget(PropertiesType& properties) : properties_(properties) {}
    
    void ApplyAnimatedProperties(const std::unordered_map<std::string, std::any>& properties) override {
        for (const auto& [name, value] : properties) {
            properties_.SetPropertyAny(name, value, true); // notify = true for updates
        }
    }
    
    PropertyType GetPropertyType(const std::string& property_name) const override {
        return properties_.GetPropertyType(property_name);
    }
    
    std::any GetPropertyValue(const std::string& property_name) const override {
        return properties_.GetPropertyAny(property_name);
    }
    
private:
    PropertiesType& properties_;
};

// Animation layer for blending multiple animations
struct AnimationLayer {
    std::shared_ptr<AnimationClip> clip;
    AnimationState state;
    float weight = 1.0f; // Blend weight [0,1]
    bool enabled = true;
    
    AnimationLayer(std::shared_ptr<AnimationClip> c) : clip(std::move(c)) {}
};

// Main animator class
template<typename PropertiesType>
class Animator {
public:
    Animator(std::unique_ptr<AnimationTarget<PropertiesType>> target)
        : target_(std::move(target)) {}
    
    // Convenience constructor for default target
    Animator(PropertiesType& properties) 
        : target_(std::make_unique<DefaultAnimationTarget<PropertiesType>>(properties)) {}
    
    // Add an animation layer
    size_t AddLayer(std::shared_ptr<AnimationClip> clip, float weight = 1.0f) {
        layers_.emplace_back(clip);
        layers_.back().weight = weight;
        return layers_.size() - 1;
    }
    
    // Remove an animation layer
    void RemoveLayer(size_t index) {
        if (index < layers_.size()) {
            layers_.erase(layers_.begin() + index);
        }
    }
    
    // Get layer by index
    AnimationLayer* GetLayer(size_t index) {
        return index < layers_.size() ? &layers_[index] : nullptr;
    }
    
    // Update all animations and apply to target
    void Update() {
        if (layers_.empty()) {
            return;
        }
        
        // Update all layer states
        for (auto& layer : layers_) {
            if (layer.enabled) {
                layer.state.Update();
            }
        }
        
        // Sample and blend all enabled layers
        std::unordered_map<std::string, std::any> blended_properties;
        std::unordered_map<std::string, float> total_weights;
        
        for (const auto& layer : layers_) {
            if (!layer.enabled || layer.weight <= 0.0f || !layer.clip) {
                continue;
            }
            
            auto sampled = layer.clip->Sample(layer.state.GetCurrentTime());
            
            for (const auto& [prop_name, value] : sampled) {
                if (layer.weight >= 1.0f && total_weights[prop_name] == 0.0f) {
                    // First layer with full weight - just use this value
                    blended_properties[prop_name] = value;
                    total_weights[prop_name] = layer.weight;
                } else {
                    // Blend with existing value
                    BlendProperty(prop_name, value, layer.weight, blended_properties, total_weights);
                }
            }
        }
        
        // Normalize weights and apply to target
        NormalizeAndApply(blended_properties, total_weights);
    }
    
    // Play a specific layer
    void Play(size_t layer_index) {
        if (auto* layer = GetLayer(layer_index)) {
            layer->state.Play();
        }
    }
    
    // Pause a specific layer
    void Pause(size_t layer_index) {
        if (auto* layer = GetLayer(layer_index)) {
            layer->state.Pause();
        }
    }
    
    // Stop a specific layer
    void Stop(size_t layer_index) {
        if (auto* layer = GetLayer(layer_index)) {
            layer->state.Stop();
        }
    }
    
    // Play all layers
    void PlayAll() {
        for (auto& layer : layers_) {
            layer.state.Play();
        }
    }
    
    // Pause all layers
    void PauseAll() {
        for (auto& layer : layers_) {
            layer.state.Pause();
        }
    }
    
    // Stop all layers
    void StopAll() {
        for (auto& layer : layers_) {
            layer.state.Stop();
        }
    }
    
    // Set layer weight
    void SetLayerWeight(size_t layer_index, float weight) {
        if (auto* layer = GetLayer(layer_index)) {
            layer->weight = std::clamp(weight, 0.0f, 1.0f);
        }
    }
    
    // Enable/disable layer
    void SetLayerEnabled(size_t layer_index, bool enabled) {
        if (auto* layer = GetLayer(layer_index)) {
            layer->enabled = enabled;
        }
    }
    
    // Get number of layers
    size_t GetLayerCount() const { return layers_.size(); }
    
    // Clear all layers
    void Clear() { layers_.clear(); }
    
private:
    void BlendProperty(const std::string& prop_name, 
                      const std::any& new_value, 
                      float weight,
                      std::unordered_map<std::string, std::any>& blended_properties,
                      std::unordered_map<std::string, float>& total_weights) {
        
        PropertyType prop_type = target_->GetPropertyType(prop_name);
        
        if (blended_properties.find(prop_name) == blended_properties.end()) {
            // First value for this property
            blended_properties[prop_name] = new_value;
            total_weights[prop_name] = weight;
            return;
        }
        
        // Get current blended value
        const auto& current_value = blended_properties[prop_name];
        float current_weight = total_weights[prop_name];
        
        // Calculate blend factor
        float total_weight = current_weight + weight;
        if (total_weight <= 0.0f) {
            return;
        }
        
        float blend_factor = weight / total_weight;
        
        // Perform type-specific blending using interpolation
        RuntimePropertyInterpolator interpolator(prop_type, current_value, new_value, EaseType::Linear);
        blended_properties[prop_name] = interpolator.interpolate(blend_factor);
        total_weights[prop_name] = total_weight;
    }
    
    void NormalizeAndApply(const std::unordered_map<std::string, std::any>& blended_properties,
                          const std::unordered_map<std::string, float>& total_weights) {
        
        std::unordered_map<std::string, std::any> final_properties;
        
        for (const auto& [prop_name, value] : blended_properties) {
            auto weight_it = total_weights.find(prop_name);
            if (weight_it != total_weights.end() && weight_it->second > 0.0f) {
                // If total weight is less than 1, blend with current property value
                if (weight_it->second < 1.0f) {
                    auto current_value = target_->GetPropertyValue(prop_name);
                    PropertyType prop_type = target_->GetPropertyType(prop_name);
                    
                    RuntimePropertyInterpolator interpolator(prop_type, current_value, value, EaseType::Linear);
                    final_properties[prop_name] = interpolator.interpolate(weight_it->second);
                } else {
                    final_properties[prop_name] = value;
                }
            }
        }
        
        // Apply to target
        if (!final_properties.empty()) {
            target_->ApplyAnimatedProperties(final_properties);
        }
    }
    
    std::unique_ptr<AnimationTarget<PropertiesType>> target_;
    std::vector<AnimationLayer> layers_;
};

// Animation builder helper for easy animation creation
template<typename PropertiesType>
class AnimationBuilder {
public:
    AnimationBuilder(const std::string& name = "Animation") 
        : clip_(std::make_shared<AnimationClip>(name)) {}
    
    // Add keyframe for a property
    template<typename T>
    AnimationBuilder& AddKeyframe(const std::string& property_name, PropertyType type, 
                                 float time, const T& value, EaseType ease = EaseType::Linear) {
        auto* track = clip_->GetTrack(property_name);
        if (!track) {
            track = &clip_->AddTrack(property_name, type);
        }
        track->AddKeyframe(time, value, ease);
        return *this;
    }
    
    // Set animation properties
    AnimationBuilder& SetDuration(float duration) {
        clip_->SetDuration(duration);
        return *this;
    }
    
    AnimationBuilder& SetLooping(bool loop) {
        clip_->SetLooping(loop);
        return *this;
    }
    
    // Build the animation clip
    std::shared_ptr<AnimationClip> Build() {
        clip_->RecalculateDuration();
        return clip_;
    }
    
private:
    std::shared_ptr<AnimationClip> clip_;
};

} // namespace vortex::animation