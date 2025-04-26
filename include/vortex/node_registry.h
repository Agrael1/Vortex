#pragma once
#include <vortex/output/window_output.h>
#include <vortex/output/ndi_output.h>

namespace vortex {
inline void RegisterHardwareNodes()
{
    vortex::WindowOutput::RegisterNode();
    vortex::NDIOutput::RegisterNode();
}
} // namespace vortex