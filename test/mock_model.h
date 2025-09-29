#include <vortex/model.h>
#include <vortex/graphics.h>
#include <vortex/nodes/node_registry.h>
#include "mock_output.h"

static const vortex::LogOptions options{
    .name = vortex::app_log_name,
    .pattern_prefix = "vortex",
};
static const vortex::LogOptions options_gfx{
    .name = vortex::graphics_log_name,
    .pattern_prefix = "vortex.graphics",
};

class Initializer
{
public:
    static void Instance()
    {
        static Initializer instance;
        (void)instance; // Prevent unused variable warning
    }

private:
    Initializer()
    {
        log_global.SetAsDefault();
        vortex::RegisterHardwareNodes();
        vortex::MockOutput::RegisterNode();
    }
    vortex::Log log_global{ options };
    vortex::Log log_graphics{ options_gfx };
};
struct InitToken {
    InitToken()
    {
        Initializer::Instance();
    }
};
