#pragma once
#include <vortex/anim/animation_clip.h>
#include <unordered_map>
#include <memory>

namespace vortex::anim {

// Animation system that integrates with your PTS timeline
class AnimationSystem
{
public:
    auto AddClip(graph::INode* node) -> AnimationClip*;
    void RemoveClip(AnimationClip* clip);

    void Play(int64_t start_pts);
    void Pause(int64_t current_pts);
    void Resume(int64_t current_pts);
    void Stop();

    // Evaluate all active clips at current timeline position
    void EvaluateAtPTS(int64_t current_pts);

private:
    std::unordered_map<uintptr_t, std::unique_ptr<AnimationClip>> clips;
};

} // namespace vortex::anim