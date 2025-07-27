#pragma once
#include <vortex/nodes/output/window_output.h>
#include <vortex/nodes/output/ndi_output.h>
#include <vortex/nodes/input/image_input.h>
#include <vortex/nodes/filter/blend.h>

namespace vortex {
inline void RegisterHardwareNodes()
{
    vortex::WindowOutput::RegisterNode();
    vortex::NDIOutput::RegisterNode();
    vortex::ImageInput::RegisterNode();
    vortex::Blend::RegisterNode();
}
} // namespace vortex