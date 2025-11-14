#pragma once
#include <wisdom/wisdom_descriptor_buffer.hpp>
#include <wisdom/wisdom.hpp>
#include <vortex/consts.h>

namespace vortex {
class Graphics;
struct DescriptorTableOffset {
    uint32_t descriptor_table_offset : 31 = 0; // Offset in bytes to the descriptor table
    uint32_t is_sampler_table : 1 = 0; // 1 - sampler table, 0 - descriptor table
};

struct DescriptorBufferInfo {
    uint32_t descriptor_size_bytes = 0; // Size of a single descriptor in bytes
    uint32_t table_alignment_bytes = 0; // Alignment of the table in bytes
    uint32_t size_bytes = 0; // Number of samplers in the table
    uint32_t offset_bytes : 31 = 0; // Number of descriptors in the table
    uint32_t is_sampler_table : 1 = 0; // 1 - sampler table, 0 - descriptor table
};

class DescriptorBufferView
{
public:
    DescriptorBufferView() = default;
    DescriptorBufferView(wis::DescriptorBuffer* desc_buffer, DescriptorBufferInfo info) noexcept
        : _desc_buffer(desc_buffer)
        , _info(info)
    {
        assert(info.size_bytes % info.table_alignment_bytes == 0 &&
               "Table must be aligned!"); // Must be aligned
    }

public:
    DescriptorBufferView SuballocateTable(uint32_t desc_count) noexcept
    {
        DescriptorBufferInfo info = _info;
        info.size_bytes = wis::aligned_size(desc_count * info.descriptor_size_bytes,
                                            info.table_alignment_bytes);

        assert(_info.size_bytes >= info.size_bytes && "Not enough space in the descriptor buffer!");

        // Advance current offset
        _info.offset_bytes += info.size_bytes;
        _info.size_bytes -= info.size_bytes;
        return DescriptorBufferView(_desc_buffer, info);
    }
    operator bool() const noexcept { return _desc_buffer != nullptr && _info.size_bytes > 0; }
    uint32_t GetSizeBytes() const noexcept { return _info.size_bytes; }

public:
    void WriteTexture(uint32_t index, wis::ShaderResourceView resource) noexcept
    {
        assert(_desc_buffer != nullptr);
        assert(index * _info.descriptor_size_bytes < _info.size_bytes);
        assert(!_info.is_sampler_table && "Cannot write texture to sampler table!");
        _desc_buffer->WriteTexture(_info.offset_bytes, index, std::move(resource));
    }

    void WriteSampler(uint32_t index, wis::SamplerView sampler) noexcept
    {
        assert(_desc_buffer != nullptr);
        assert(index * _info.descriptor_size_bytes < _info.size_bytes);
        assert(_info.is_sampler_table && "Cannot write sampler to descriptor table!");
        _desc_buffer->WriteSampler(_info.offset_bytes, index, std::move(sampler));
    }

    void WriteRWBuffer(uint32_t index,
                       wis::BufferView buffer,
                       uint32_t stride,
                       uint32_t element_count) noexcept
    {
        assert(_desc_buffer != nullptr);
        assert(index * _info.descriptor_size_bytes < _info.size_bytes);
        assert(!_info.is_sampler_table && "Cannot write RW buffer to sampler table!");
        _desc_buffer->WriteRWStructuredBuffer(_info.offset_bytes,
                                              index,
                                              std::move(buffer),
                                              stride,
                                              element_count,
                                              0);
    }

    void BindOffset(const vortex::Graphics& gfx,
                    wis::CommandList& cmd_list,
                    wis::RootSignatureView root,
                    uint32_t root_table_index) const noexcept;
    void BindComputeOffset(const vortex::Graphics& gfx,
                           wis::CommandList& cmd_list,
                           wis::RootSignatureView root,
                           uint32_t root_table_index) const noexcept;

private:
#ifdef VORTEX_DX12
    void DX12SetComputeDescriptorTableOffset(wis::DX12CommandListView cmd_list,
                                             wis::DX12RootSignatureView root_signature,
                                             uint32_t root_table_index,
                                             wis::DX12DescriptorBufferGPUView buffer,
                                             uint32_t table_aligned_byte_offset) const noexcept
    {
        auto handle = std::get<0>(buffer);
        auto* list = reinterpret_cast<ID3D12GraphicsCommandList*>(std::get<0>(cmd_list));
        uint32_t root_table_offset = std::get<2>(
                root_signature); // offset of the root table in the root signature (after push
                                 // constants and push desriptors)
        list->SetComputeRootDescriptorTable(
                root_table_offset + root_table_index,
                CD3DX12_GPU_DESCRIPTOR_HANDLE(handle, table_aligned_byte_offset));
    }
#endif

private:
    wis::DescriptorBuffer* _desc_buffer = nullptr;
    DescriptorBufferInfo _info = {};
};

class DescriptorBuffer
{
public:
    DescriptorBuffer() = default;
    explicit DescriptorBuffer(const vortex::Graphics& gfx,
                              uint32_t desc_batch_size = descriptor_batch_size,
                              uint32_t samp_batch_size = sampler_batch_size);

public:
    operator bool() const noexcept { return bool(_desc_buffer) && bool(_sampler_buffer); }

    // TODO: Encapsulate
    template<typename Self>
    auto& GetCurrentDescriptorBuffer(this Self&& self) noexcept
    {
        return *self._current_desc;
    }
    template<typename Self>
    auto& GetSamplerBuffer(this Self&& self) noexcept
    {
        return self._sampler_buffer;
    }

    void BindBuffers(const vortex::Graphics& gfx, wis::CommandList& cmd_list) const noexcept;
    void BindOffsets(const vortex::Graphics& gfx,
                     wis::CommandList& cmd_list,
                     wis::RootSignatureView root,
                     uint32_t frame,
                     std::span<const DescriptorTableOffset> offsets) const noexcept;

    DescriptorBufferView DescBufferView(uint32_t frame_index) noexcept
    {
        assert(frame_index < max_frames_in_flight);
        DescriptorBufferInfo info = _desc_info;
        info.size_bytes = _desc_info.offset_bytes; // Use offset as size for a single batch
        info.offset_bytes *= frame_index;
        return DescriptorBufferView(&_desc_buffer, info);
    }
    DescriptorBufferView SamplerBufferView(uint32_t frame_index) noexcept
    {
        assert(frame_index < max_frames_in_flight);
        DescriptorBufferInfo info = _sampler_info;
        info.size_bytes = _sampler_info.offset_bytes; // Use offset as size for a single batch
        info.offset_bytes *= frame_index;
        return DescriptorBufferView(&_sampler_buffer, info);
    }

private:
    wis::DescriptorBuffer _desc_buffer = {}; // Buffer for reallocation. Best
                                             // case is that it won't be
                                             // reallocated.
    wis::DescriptorBuffer _sampler_buffer = {}; // Buffer for samplers

    DescriptorBufferInfo _desc_info = {};
    DescriptorBufferInfo _sampler_info = {};
};

} // namespace vortex