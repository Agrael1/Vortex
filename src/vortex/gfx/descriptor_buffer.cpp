#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/graphics.h>
#include <vortex/util/common.h>

vortex::DescriptorBuffer::DescriptorBuffer(const vortex::Graphics& gfx,
                                           uint32_t desc_batch_size,
                                           uint32_t samp_batch_size)
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    uint32_t desc_size = uint32_t(desc_ext.GetDescriptorSize(wis::DescriptorHeapType::Descriptor));
    uint32_t sampler_size = uint32_t(desc_ext.GetDescriptorSize(wis::DescriptorHeapType::Sampler));
    uint32_t desc_alignment = uint32_t(
            desc_ext.GetDescriptorTableAlignment(wis::DescriptorHeapType::Descriptor));
    uint32_t sampler_alignment = uint32_t(
            desc_ext.GetDescriptorTableAlignment(wis::DescriptorHeapType::Sampler));

    _desc_info = {
        .descriptor_size_bytes = desc_size,
        .table_alignment_bytes = desc_alignment,
        .size_bytes = wis::aligned_size(desc_batch_size * desc_size * max_frames_in_flight,
                                        desc_alignment),

        // Use to store the table offset for a single batch
        .offset_bytes = wis::aligned_size(desc_batch_size * desc_size, desc_alignment),
        .is_sampler_table = false,
    };
    _sampler_info = {
        .descriptor_size_bytes = sampler_size,
        .table_alignment_bytes = sampler_alignment,
        .size_bytes = wis::aligned_size(samp_batch_size * sampler_size * max_frames_in_flight,
                                        sampler_alignment),

        // Use to store the table offset for a single batch
        .offset_bytes = wis::aligned_size(samp_batch_size * sampler_size, sampler_alignment),
        .is_sampler_table = true,
    };

    wis::Result result = wis::success;

    // Create descriptor buffer
    _desc_buffer = desc_ext.CreateDescriptorBuffer(result,
                                                   wis::DescriptorHeapType::Descriptor,
                                                   wis::DescriptorMemory::ShaderVisible,
                                                   _desc_info.size_bytes);
    if (!vortex::success(result)) {
        vortex::error("DescriptorBuffer: Failed to create descriptor buffer: {}", result.error);
        return;
    }

    _sampler_buffer = desc_ext.CreateDescriptorBuffer(result,
                                                      wis::DescriptorHeapType::Sampler,
                                                      wis::DescriptorMemory::ShaderVisible,
                                                      _sampler_info.size_bytes);
    if (!vortex::success(result)) {
        vortex::error("DescriptorBuffer: Failed to create sampler buffer: {}", result.error);
    }
}

void vortex::DescriptorBuffer::BindBuffers(const vortex::Graphics& gfx,
                                           wis::CommandList& cmd_list) const noexcept
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    desc_ext.SetDescriptorBuffers(cmd_list, _desc_buffer, _sampler_buffer);
}

void vortex::DescriptorBuffer::BindOffsets(
        const vortex::Graphics& gfx,
        wis::CommandList& cmd_list,
        wis::RootSignatureView root,
        uint32_t frame,
        std::span<const DescriptorTableOffset> offsets) const noexcept
{
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    for (uint32_t i = 0; i < offsets.size(); i++) {
        const auto& offset = offsets[i];
        uint64_t offset_size = offset.is_sampler_table ? _sampler_info.offset_bytes
                                                       : _desc_info.offset_bytes;

        auto& buffer = offset.is_sampler_table ? _sampler_buffer : _desc_buffer;
        desc_ext.SetDescriptorTableOffset(cmd_list,
                                          root,
                                          i,
                                          buffer,
                                          offset.descriptor_table_offset + (frame * offset_size));
    }
}

void vortex::DescriptorBufferView::BindOffset(const vortex::Graphics& gfx,
                                              wis::CommandList& cmd_list,
                                              wis::RootSignatureView root,
                                              uint32_t root_table_index) const noexcept
{
    assert(_desc_buffer != nullptr);
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    desc_ext.SetDescriptorTableOffset(cmd_list,
                                      root,
                                      root_table_index,
                                      *_desc_buffer,
                                      _info.offset_bytes);
}

void vortex::DescriptorBufferView::BindComputeOffset(const vortex::Graphics& gfx,
                                                     wis::CommandList& cmd_list,
                                                     wis::RootSignatureView root,
                                                     uint32_t root_table_index) const noexcept
{
    assert(_desc_buffer != nullptr);
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    DX12SetComputeDescriptorTableOffset(cmd_list,
                                        root,
                                        root_table_index,
                                        *_desc_buffer,
                                        _info.offset_bytes);
}
