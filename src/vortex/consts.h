#pragma once
#include <cstdint>

namespace vortex {
inline constexpr uint32_t max_frames_in_flight = 2u; // Maximum number of frames in flight for all output channels
inline constexpr uint32_t descriptor_batch_size = 1024u; // Maximum number of descriptors in a single batch for all output channels
inline constexpr uint32_t sampler_batch_size = 64u; // Maximum number of samplers in a single batch for all output channels
}

#if defined(WISDOM_DX12) && !defined(WISDOM_FORCE_VULKAN)
#define VORTEX_DX12
#else
#define VORTEX_VULKAN
#endif