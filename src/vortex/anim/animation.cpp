#include <vortex/anim/animation.h>
#include <vortex/graph/interfaces.h>
#include <vortex/util/log.h>
#include <vortex/util/interp/easing.h>
#include <algorithm>
#include <cmath>
#include "animation_clip.h"

namespace vortex::anim {

AnimationClip* AnimationSystem::AddClip(graph::INode* node)
{
    std::unique_ptr<AnimationClip> clip = std::make_unique<AnimationClip>(node);
    AnimationClip* clip_ptr = clip.get();
    uintptr_t clip_id = reinterpret_cast<uintptr_t>(clip_ptr);
    clips[clip_id] = std::move(clip);
    return clip_ptr;
}

void AnimationSystem::RemoveClip(AnimationClip* clip)
{
    if (!clip) {
        return;
    }
    uintptr_t clip_id = reinterpret_cast<uintptr_t>(clip);
    clips.erase(clip_id);
}

void AnimationSystem::Play(int64_t start_pts)
{
    for (auto& [id, clip] : clips) {
        clip->Play(start_pts);
    }
}

void AnimationSystem::Pause(int64_t current_pts)
{
    for (auto& [id, clip] : clips) {
        clip->Pause(current_pts);
    }
}

void AnimationSystem::Resume(int64_t current_pts)
{
    for (auto& [id, clip] : clips) {
        clip->Resume(current_pts);
    }
}

void AnimationSystem::Stop()
{
    for (auto& [id, clip] : clips) {
        clip->Stop();
    }
}

void AnimationSystem::EvaluateAtPTS(int64_t current_pts)
{
    for (auto& [id, clip] : clips) {
        clip->EvaluateAtTime(current_pts);
    }
}

} // namespace vortex::anim


