module;
#include <format>
#include <stdexcept>
export module vortex.graphics;

import wisdom;
import wisdom.debug;
import wisdom.descriptor_buffer;
import wisdom.extended_allocation;
import vortex.log;

export namespace vortex {
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
    void OnDebugMessage(wis::Severity severity, const char* message)
    {
        switch (severity) {
        case wis::Severity::Info:
            _log.info(message);
            break;
        case wis::Severity::Warning:
            _log.warn(message);
            break;
        case wis::Severity::Error:
            _log.error(message);
            break;
        case wis::Severity::Critical:
            _log.critical(message);
            break;
        case wis::Severity::Trace:
            _log.trace(message);
            break;
        case wis::Severity::Debug:
            _log.debug(message);
            break;
        default:
            break;
        }
    }

private:
    vortex::LogView _log;
    wis::DebugMessenger _messenger;
};

inline bool succeded(wis::Result result) noexcept
{
    return int(result.status) >= 0;
}

export class Graphics
{
public:
    Graphics(wis::FactoryExtension* platform_extension, bool debug_extension)
    {
        CreateDevice(platform_extension, debug_extension);
    }

private:
    void CreateDevice(wis::FactoryExtension* platform_extension, bool debug_extension)
    {
        wis::Result result = wis::success;
        wis::DebugExtension debug_ext;
        vortex::LogView log = vortex::GetLog(vortex::graphics_log_name);

        wis::FactoryExtension* extensions[] = { platform_extension, &debug_ext };
        uint32_t extension_count = std::size(extensions) - !debug_extension - !platform_extension; // subtract 1 for each missing extension
        
        wis::Factory factory = wis::CreateFactory(result, debug_extension, extensions + !platform_extension, extension_count);
        if (!succeded(result)) {
            throw std::runtime_error(std::format("Failed to create factory: {}", result.error));
        }

        if (debug_extension) {
            std::construct_at<Debug>(&_debug, result, debug_ext, log);
            if (!succeded(result)) {
                log.error(std::format("Failed to create debug extension: {}", result.error)); // continue without debug extension
            }
        }

        wis::Adapter adapter;
        wis::DeviceExtension* device_extensions[] = {
            &_descriptor_buffer_ext,
            &_extended_allocation_ext
        };

        for (uint32_t i = 0;; ++i) {
            adapter = factory.GetAdapter(result, i, wis::AdapterPreference::Performance);
            if (!succeded(result)) { // no more adapters
                throw std::runtime_error(std::format("Failed to get adapter: {}", result.error));
            }

            _device = wis::CreateDevice(result, adapter, device_extensions, std::size(device_extensions));
            if (succeded(result)) {
                log.info(std::format("Created device on adapter: {}", i));

                wis::AdapterDesc desc;
                result = adapter.GetDesc(&desc); // almost always succeeds

                log.info(std::format("Adapter description: {}", std::string_view{ desc.description.data() }));
                return;
            }
        }
    }

private:
    Debug _debug;
    wis::Device _device;

    wis::DescriptorBufferExtension _descriptor_buffer_ext;
    wis::ExtendedAllocation _extended_allocation_ext;
};
} // namespace vortex