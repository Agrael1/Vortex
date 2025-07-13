#pragma once
#include <wisdom/wisdom_descriptor_buffer.hpp>
#include <vortex/gfx/consts.h>

namespace vortex {
class Graphics;
struct DescriptorTableOffset {
    uint32_t descriptor_table_offset : 31 = 0; // Offset in bytes to the descriptor table
    uint32_t is_sampler_table : 1 = 0; // 1 if this is a sampler table, 0 if this is a descriptor table
};
class DescriptorBuffer
{
public:
    DescriptorBuffer() = default;
    explicit DescriptorBuffer(const vortex::Graphics& gfx);

public:
    operator bool() const noexcept
    {
        return _current_desc != nullptr && bool(_sampler_buffer);
    }

    // TODO: Encapsulate
    template<typename Self>
    decltype(auto) GetCurrentDescriptorBuffer(this Self&& self) noexcept
    {
        return *self._current_desc;
    }
    template<typename Self>
    decltype(auto) GetSamplerBuffer(this Self&& self) noexcept
    {
        return self._sampler_buffer;
    }

    void BindBuffers(const vortex::Graphics& gfx, wis::CommandList& cmd_list) const noexcept;
    void BindOffsets(const vortex::Graphics& gfx, wis::CommandList& cmd_list, wis::RootSignatureView root, uint32_t frame, std::span<const DescriptorTableOffset> offsets) const noexcept;

    void Increase(); // Won't do until necessary

private:
    wis::DescriptorBuffer _desc_buffer[max_frames_in_flight] = {}; // Buffer for reallocation. Best case is that it won't be reallocated.
    wis::DescriptorBuffer _sampler_buffer = {}; // Buffer for samplers
    wis::DescriptorBuffer* _current_desc = nullptr; // Current buffer for descriptor allocation
    uint64_t _current_desc_table_size = 0; // size of the current descriptor table in bytes
    uint64_t _sampler_table_size = 0; // size of the current sampler table in bytes
};
} // namespace vortex