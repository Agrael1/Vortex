#pragma once
#include <vortex/node.h>

namespace vortex {
class GraphModel
{
public:
    GraphModel() = default;

public:
    void CreateNode(const vortex::Graphics& gfx)
    {
        _nodes.emplace_back(std::make_unique<vortex::ImageInput>(
                gfx,
                vortex::ImageInput::ImageParams{
                        .data = {
                                .image_path = "C:/Users/Agrae/Downloads/HDR.jpg",
                                .crop_rect = { 0.f, 0.f, 1.f, 1.f }, // Full image
                                .size = { 1280, 720 },
                                .origin = { 0, 0 },
                                .rotation = { 0.f, 0.f } } }));
    }

private:
    std::vector<std::unique_ptr<vortex::INode>> _nodes;
    std::vector<std::unique_ptr<vortex::INode>> _outputs;
};
} // namespace vortex