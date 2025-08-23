#include <format>
#include <fstream>
#include <stdexcept>
#include <iterator>
#include <vortex/util/common.h>
#include <vortex/graphics.h>

void vortex::Debug::OnDebugMessage(wis::Severity severity, const char* message)
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

wis::Shader vortex::Graphics::LoadShader(std::filesystem::path path) const
{
    wis::Result result = wis::success;
    vortex::LogView log = vortex::GetLog(vortex::graphics_log_name);
    if constexpr (wis::shader_intermediate == wis::ShaderIntermediate::DXIL) {
        path += u".cso";
    } else {
        path += u".spv";
    }

    std::string path_string = path.string();

    // Check if the file exists
    if (!std::filesystem::exists(path)) {
        log.error("Graphics::LoadShader: Shader file does not exist: {}", path_string);
        return {};
    }

    // Load the shader from the file
    std::ifstream shader{ path, std::ios::binary };
    if (!shader.is_open()) {
        log.error("Graphics::LoadShader: Failed to open shader file: {}", path_string);
        return {};
    }
    std::vector<char> shader_data((std::istreambuf_iterator<char>(shader)), std::istreambuf_iterator<char>());

    wis::Shader result_shader = _device.CreateShader(result, shader_data.data(), shader_data.size());
    if (!vortex::success(result)) {
        log.error("Graphics::LoadShader: Failed to load shader from {}: {}", path_string, result.error);
    }
    return result_shader;
}

void vortex::Graphics::CreateDevice(bool debug_extension)
{
    wis::Result result = wis::success;
    wis::DebugExtension debug_ext;
    vortex::LogView log = vortex::GetLog(vortex::graphics_log_name);

    wis::FactoryExtension* extensions[] = { _platform.GetExtension(), &debug_ext };
    uint32_t extension_count = std::size(extensions) - !debug_extension; // subtract 1 for each missing extension

    // Create the factory
    wis::Factory factory = wis::CreateFactory(result, debug_extension, extensions, extension_count);
    if (!success(result)) {
        throw std::runtime_error(std::format("Failed to create factory: {}", result.error));
    }

    if (debug_extension) {
        std::construct_at<Debug>(&_debug, result, debug_ext, log);
        if (!success(result)) {
            log.error("Failed to create debug extension: {}", result.error); // continue without debug extension
        }
    }

    wis::Adapter adapter;
    wis::DeviceExtension* device_extensions[] = {
        &_descriptor_buffer_ext,
        &_extended_allocation_ext
    };

    // Get the first adapter that supports the requested features
    for (uint32_t i = 0;; ++i) {
        adapter = factory.GetAdapter(result, i, wis::AdapterPreference::Performance);
        if (!success(result)) { // no more adapters
            throw std::runtime_error(std::format("Failed to get adapter: {}", result.error));
        }

        _device = wis::CreateDevice(result, adapter, device_extensions, std::size(device_extensions));
        if (success(result)) {
            log.info("Created device on adapter: {}", i);

            wis::AdapterDesc desc;
            result = adapter.GetDesc(&desc); // almost always succeeds

            log.info("Adapter description: {}", std::string_view{ desc.description.data() });
            break;
        }
    }

    // Create the main queue
    _main_queue = _device.CreateCommandQueue(result, wis::QueueType::Graphics);
    if (!success(result)) {
        throw std::runtime_error(std::format("Failed to create main queue: {}", result.error));
    }

    // Create the resource allocator
    _allocator = _device.CreateAllocator(result);
    if (!success(result)) {
        throw std::runtime_error(std::format("Failed to create resource allocator: {}", result.error));
    }

    // Create the fence for throttling
    _fence = _device.CreateFence(result);
    if (!success(result)) {
        throw std::runtime_error(std::format("Failed to create fence: {}", result.error));
    }
}
