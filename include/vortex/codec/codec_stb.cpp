#include <vortex/codec/codec_stb.h>
#include <vortex/codec/codec_common.h>
#include <util/lib/stb_image.h>
#include <util/log.h>
#include <util/common.h>

wis::Texture vortex::codec::STBCodec::LoadTexture(const vortex::Graphics& gfx, const std::filesystem::path& file_path)
{
    ImageInfo image_info{};

    int width = 0, height = 0, channels = 0, desired_channels = 0;
    if (!std::filesystem::exists(file_path)) {
        vortex::error("STBCodec::LoadTexture: File does not exist: {}", file_path.string());
    }

    unique_file file{ fopen(file_path.string().c_str(), "rb") };
    if (!file) {
        vortex::error("STBCodec::LoadTexture: Unable to open file: {}", file_path.string());
        return {};
    }

    int result = stbi_info_from_file(file.get(), &width, &height, &channels);
    if (result == 0) {
        // get the error message from stb_image
        const char* error_msg = stbi_failure_reason();
        vortex::error("STBCodec::LoadTexture: Failed to get image info from file: {}. Error: {}", file_path.string(), error_msg);
        return {};
    }

    desired_channels = channels == 3 ? 4 : channels; // Convert RGB to RGBA

    // Check if the file is HDR
    using unique_data = unique_any<void*, stbi_image_free>;

    unique_data data = nullptr;
    if (stbi_is_hdr_from_file(file.get())) {
        data = stbi_loadf_from_file(file.get(), &width, &height, &channels, desired_channels);
        image_info.bit_depth = 32; // HDR images are typically 32-bit float
    } else if (stbi_is_16_bit_from_file(file.get())) {
        data = stbi_load_from_file_16(file.get(), &width, &height, &channels, desired_channels);
        image_info.bit_depth = 16; // 16-bit images
    } else {
        data = stbi_load_from_file(file.get(), &width, &height, &channels, desired_channels);
        image_info.bit_depth = 8; // Standard 8-bit images
    }

    if (!data) {
        // get the error message from stb_image
        const char* error_msg = stbi_failure_reason();
        vortex::error("STBCodec::LoadTexture: Failed to load image from file: {}. Error: {}", file_path.string(), error_msg);
        return {};
    }

    image_info.width = width;
    image_info.height = height;
    image_info.channels = channels;



    return {};
}