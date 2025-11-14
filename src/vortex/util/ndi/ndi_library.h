#pragma once
#ifdef NDI_AVAILABLE
#include <Processing.NDI.Lib.h>
#endif // NDI_AVAILABLE

#include <vortex/util/log.h>
#include <wisdom/wisdom.hpp>

namespace vortex {
class NDILibrary
{
public:
    NDILibrary()
    {
        int result = 0;
#ifdef NDI_AVAILABLE
        result = NDIlib_initialize();
#endif
        if (result == 0) {
            vortex::error("NDI library initialization failed");
        } else {
            vortex::info("NDI library initialized successfully");
        }
    }
    ~NDILibrary()
    {
#ifdef NDI_AVAILABLE
        NDIlib_destroy();
#endif
    }
};

#ifdef NDI_AVAILABLE
constexpr inline NDIlib_FourCC_video_type_e GetNDIFormat(wis::DataFormat format) noexcept
{
    switch (format) {
    case wis::DataFormat::RGBA8Unorm:
        return NDIlib_FourCC_video_type_UYVY; // Use UYVY for better NDI performance
    case wis::DataFormat::BGRA8Unorm:
        return NDIlib_FourCC_video_type_UYVY; // Use UYVY for better NDI performance
    default:
        return NDIlib_FourCC_video_type_UYVY;
    }
}

struct NDISendInstance {
    NDISendInstance() = default;
    NDISendInstance(std::string_view name) { instance = CreateSendInstance(name); }
    NDISendInstance(const NDISendInstance&) = delete;
    NDISendInstance& operator=(const NDISendInstance&) = delete;
    NDISendInstance(NDISendInstance&& other) noexcept
        : instance(other.instance)
    {
        other.instance = nullptr;
    }
    NDISendInstance& operator=(NDISendInstance&& other) noexcept
    {
        if (this != &other) {
            if (instance) {
                NDIlib_send_destroy(instance);
            }
            instance = other.instance;
            other.instance = nullptr;
        }
        return *this;
    }
    ~NDISendInstance() { Reset(); }

public:
    operator NDIlib_send_instance_t() const noexcept { return instance; }
    operator bool() const noexcept { return instance != nullptr; }
    void Reset()
    {
        if (instance) {
            NDIlib_send_destroy(instance);
            instance = nullptr;
        }
    }
    void Recreate(std::string_view name)
    {
        Reset();
        instance = CreateSendInstance(name);
    }

private:
    static NDIlib_send_instance_t CreateSendInstance(std::string_view name)
    {
        NDIlib_send_create_t create_desc = {};
        create_desc.p_ndi_name = name.data();
        create_desc.clock_video = true;
        create_desc.clock_audio = false;
        auto send_instance = NDIlib_send_create(&create_desc);
        if (!send_instance) {
            vortex::error("Failed to create NDI send instance");
        }
        return send_instance;
    }

private:
    NDIlib_send_instance_t instance = nullptr;
};
#endif // NDI_AVAILABLE
} // namespace vortex