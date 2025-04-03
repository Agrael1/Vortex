module;
#ifdef NDI_AVAILABLE
#include <Processing.NDI.Lib.h>
#endif // NDI_AVAILABLE

export module vortex.ndi_output;

import vortex.log;
import vortex.graphics;

namespace vortex {
export class NDILibrary
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
export class NDIOutput
{
public:
    NDIOutput(const char* name)
    {
#ifdef NDI_AVAILABLE
        NDIlib_send_create_t create_desc = {};
        create_desc.p_ndi_name = name;
        send_instance = NDIlib_send_create(&create_desc);
        if (!send_instance) {
            vortex::error("Failed to create NDI send instance");
        } else {
            vortex::info("NDI send instance created successfully");
        }
        video_frame.xres = 1280;
        video_frame.yres = 720;
        video_frame.FourCC = NDIlib_FourCC_video_type_RGBA;
        video_frame.p_data = nullptr;
#endif
    }

private:
#ifdef NDI_AVAILABLE
    NDIlib_send_instance_t send_instance = nullptr;

    // TODO:
    // 1) Make a wisdom buffer for the video frame data (readback tex)
    // 2) Make swapchain that can be used to render to the NDI output using async api
    NDIlib_video_frame_v2_t video_frame = {};
#endif
};
} // namespace vortex