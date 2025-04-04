export module vortex.node_registry;

import vortex.ndi_output;
import vortex.sdl;

namespace vortex {
export void RegisterHardwareNodes()
{
    WindowOutput::RegisterNode();
    NDIOutput::RegisterNode();
}
} // namespace vortex