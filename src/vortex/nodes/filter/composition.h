#pragma once
#include <vortex/graph/interfaces.h>

namespace vortex {
// Composition node is a filter that combines multiple inputs into a single output.
class Composition : public vortex::graph::FilterImpl<Composition, void, graph::Sink::dynamic_index, 1>
{

};
} // namespace vortex