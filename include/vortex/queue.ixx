module;
// #ifdef __INTELLISENSE__
// #include <wisdom/wisdom.hpp>
// #endif
#include <expected>
#include <string_view>
export module vortex.queue;

import vortex.graphics;
import wisdom;

namespace vortex {
export class RenderVisitor
{
};

export class Queue
{
public:
    Queue() = default;
    Queue(wis::CommandQueue&& queue)
        : _queue(std::move(queue))
    {
    }

private:
    wis::CommandQueue _queue;
};

export std::expected<Queue, std::string_view> CreateQueue(const vortex::Graphics& gfx, wis::QueueType type)
{
    wis::Result result = wis::success;
    wis::CommandQueue queue = gfx.GetDevice().CreateCommandQueue(result, type);
    if (!vortex::succeded(result)) {
        return std::unexpected{ std::string_view{ result.error } };
    }
    return Queue{ std::move(queue) };
}

export std::expected<Queue, std::string_view> CreateRenderQueue(const vortex::Graphics& gfx)
{
    return CreateQueue(gfx, wis::QueueType::Graphics);
}
export std::expected<Queue, std::string_view> CreateComputeQueue(const vortex::Graphics& gfx)
{
    return CreateQueue(gfx, wis::QueueType::Compute);
}
export std::expected<vortex::Queue, std::string_view> CreateCopyQueue(const vortex::Graphics& gfx)
{
    return CreateQueue(gfx, wis::QueueType::Copy);
}
} // namespace vortex
