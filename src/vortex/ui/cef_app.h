#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_app.h>

namespace vortex::ui {
class VortexCefApp : public CefImplements<VortexCefApp, CefApp>
{
};
}