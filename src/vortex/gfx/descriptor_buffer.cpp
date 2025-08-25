#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/graphics.h>
#include <vortex/util/common.h>

vortex::DescriptorBuffer::DescriptorBuffer(const vortex::Graphics& gfx)
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    uint64_t desc_size = desc_ext.GetDescriptorSize(wis::DescriptorHeapType::Descriptor);
    uint64_t sampler_size = desc_ext.GetDescriptorSize(wis::DescriptorHeapType::Sampler);

    uint64_t desc_alignment = desc_ext.GetDescriptorTableAlignment(wis::DescriptorHeapType::Descriptor);
    uint64_t sampler_alignment = desc_ext.GetDescriptorTableAlignment(wis::DescriptorHeapType::Sampler);

    _current_desc_table_size = descriptor_batch_size * desc_size;
    _sampler_table_size = sampler_batch_size * sampler_size;

    uint64_t desc_buffer_size = wis::aligned_size(_current_desc_table_size * max_frames_in_flight, desc_alignment);
    uint64_t sampler_buffer_size = wis::aligned_size(_sampler_table_size * max_frames_in_flight, sampler_alignment);

    wis::Result result = wis::success;

    // Create
    _desc_buffer[0] = desc_ext.CreateDescriptorBuffer(result, wis::DescriptorHeapType::Descriptor, wis::DescriptorMemory::ShaderVisible, desc_buffer_size);
    if (!vortex::success(result)) {
        vortex::error("DescriptorBuffer: Failed to create descriptor buffer: {}", result.error);
        return;
    }

    _desc_buffer[1] = desc_ext.CreateDescriptorBuffer(result, wis::DescriptorHeapType::Descriptor, wis::DescriptorMemory::ShaderVisible, desc_buffer_size);
    if (!vortex::success(result)) {
        vortex::error("DescriptorBuffer: Failed to create descriptor buffer: {}", result.error);
        return;
    }

    _sampler_buffer = desc_ext.CreateDescriptorBuffer(result, wis::DescriptorHeapType::Sampler, wis::DescriptorMemory::ShaderVisible, sampler_buffer_size);
    if (!vortex::success(result)) {
        vortex::error("DescriptorBuffer: Failed to create sampler buffer: {}", result.error);
        return;
    }

    // Initialize current descriptor buffer
    _current_desc = &_desc_buffer[0];
}

void vortex::DescriptorBuffer::BindBuffers(const vortex::Graphics& gfx, wis::CommandList& cmd_list) const noexcept
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    desc_ext.SetDescriptorBuffers(cmd_list, *_current_desc, _sampler_buffer);
}

void vortex::DescriptorBuffer::BindOffsets(const vortex::Graphics& gfx, wis::CommandList& cmd_list, wis::RootSignatureView root, uint32_t frame, std::span<const DescriptorTableOffset> offsets) const noexcept
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    for (uint32_t i = 0; i < offsets.size(); i++) {
        const auto& offset = offsets[i];
        uint64_t offset_size = offset.is_sampler_table ? _sampler_table_size : _current_desc_table_size;
        auto& buffer = offset.is_sampler_table ? _sampler_buffer : *_current_desc;
        desc_ext.SetDescriptorTableOffset(cmd_list, root, i, buffer, offset.descriptor_table_offset + (frame * offset_size));
    }
}
