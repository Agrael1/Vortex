#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_debug.hpp>
#include <wisdom/wisdom_descriptor_buffer.hpp>
#include <wisdom/wisdom_extended_allocation.hpp>
#include <vortex/util/log.h>
#include <vortex/platform.h>

namespace vortex {
class Debug
{
    static void DebugCallback(wis::Severity severity, const char* message, void* user_data)
    {
        auto debug = static_cast<Debug*>(user_data);
        debug->OnDebugMessage(severity, message);
    }

public:
    Debug() = default;
    Debug(wis::Result& result, wis::DebugExtension& debug_ext, vortex::LogView log)
        : _log(log)
        , _messenger(debug_ext.CreateDebugMessenger(result, &Debug::DebugCallback, this))
    {
    }

public:
    void OnDebugMessage(wis::Severity severity, const char* message);

private:
    vortex::LogView _log;
    wis::DebugMessenger _messenger;
};

class Graphics
{
public:
    Graphics(bool debug_extension)
    {
        CreateDevice(debug_extension);
    }

public:
    const wis::Device& GetDevice() const noexcept
    {
        return _device;
    }
    const vortex::PlatformExtension& GetPlatform() const noexcept
    {
        return _platform;
    }
    const wis::CommandQueue& GetMainQueue() const noexcept
    {
        return _main_queue;
    }
    const wis::ResourceAllocator& GetAllocator() const noexcept
    {
        return _allocator;
    }
    const wis::ExtendedAllocation& GetExtendedAllocation() const noexcept
    {
        return _extended_allocation_ext;
    }
    const wis::DescriptorBufferExtension& GetDescriptorBufferExtension() const noexcept
    {
        return _descriptor_buffer_ext;
    }

public:
    wis::Shader LoadShader(std::filesystem::path path) const;

private:
    void CreateDevice(bool debug_extension);

private:
    Debug _debug;
    wis::Device _device;
    wis::CommandQueue _main_queue;
    wis::ResourceAllocator _allocator;

    vortex::PlatformExtension _platform;
    wis::DescriptorBufferExtension _descriptor_buffer_ext;
    wis::ExtendedAllocation _extended_allocation_ext;
};

} // namespace vortex