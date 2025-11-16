#include <vortex/nodes/node_registry.h>

#ifdef NDI_AVAILABLE
#include <vortex/nodes/output/ndi_output.h>
#endif // NDI_AVAILABLE

#include <vortex/nodes/output/window_output.h>
#include <vortex/nodes/input/image_input.h>
#include <vortex/nodes/input/stream_input.h>
#include <vortex/nodes/filter/blend.h>
#include <vortex/nodes/filter/select.h>
#include <vortex/nodes/filter/transform.h>
#include <vortex/nodes/filter/color_correction.h>

void vortex::RegisterHardwareNodes()
{
#ifdef NDI_AVAILABLE
    vortex::NDIOutput::RegisterNode();
#endif // NDI_AVAILABLE

    vortex::WindowOutput::RegisterNode();
    vortex::ImageInput::RegisterNode();
    vortex::StreamInput::RegisterNode();
    vortex::Blend::RegisterNode();
    vortex::Select::RegisterNode();
    vortex::Transform::RegisterNode();
    vortex::ColorCorrection::RegisterNode();
}
