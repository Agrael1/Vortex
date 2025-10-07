#pragma once
#ifdef NDI_AVAILABLE
#include <vortex/nodes/output/ndi_output.h>
#endif // NDI_AVAILABLE

#include <vortex/nodes/output/window_output.h>
#include <vortex/nodes/input/image_input.h>
#include <vortex/nodes/input/stream_input.h>
#include <vortex/nodes/filter/blend.h>
#include <vortex/nodes/filter/select.h>

namespace vortex {
inline void RegisterHardwareNodes()
{
#ifdef NDI_AVAILABLE
    vortex::NDIOutput::RegisterNode();
#endif // NDI_AVAILABLE

    vortex::WindowOutput::RegisterNode();
    vortex::ImageInput::RegisterNode();
    vortex::StreamInput::RegisterNode();
    vortex::Blend::RegisterNode();
    vortex::Select::RegisterNode();
}
} // namespace vortex