#pragma once
#include <cstdint>

namespace vortex::codec {
enum class ColorComponents : uint32_t{
    Grayscale = 0, // Single channel grayscale
    RGB = 1 << 1, // Three channels: Red, Green, Blue
    Alpha = 1 << 2, // Single channel alpha (transparency)

    // Combinations of channels
    GrayscaleAlpha = Grayscale | Alpha, // Two channels: Grayscale, Alpha
    RGBA = RGB | Alpha, // Four channels: Red, Green, Blue, Alpha
};

struct ImageInfo {
    uint32_t width; // Width of the image in pixels
    uint32_t height; // Height of the image in pixels
    uint32_t row_stride; // Number of bytes per row (including padding if any)
    uint32_t channels; // Number of color channels (e.g., 3 for RGB, 4 for RGBA)
    uint32_t bit_depth; // Bit depth per channel (e.g., 8, 16)
    ColorComponents color_components; // Color components used in the image
};
} // namespace vortex::codec
