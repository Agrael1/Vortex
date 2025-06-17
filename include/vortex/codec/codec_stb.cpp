#include <vortex/codec/codec_stb.h>
#include <vortex/codec/codec_common.h>
#include <util/lib/stb_image.h>
#include <util/log.h>
#include <util/common.h>
#include <vortex/graphics.h>

constexpr wis::DataFormat determine_format(vortex::codec::ImageInfo image_info) noexcept
{
    using enum vortex::codec::ColorComponents;
    switch (image_info.color_components) {
    case Grayscale:
        if (image_info.bit_depth == 32) {
            return wis::DataFormat::R32Float; // Single channel 32-bit float
        } else if (image_info.bit_depth == 16) {
            return wis::DataFormat::R16Unorm; // Single channel 16-bit
        }
        return wis::DataFormat::R8Unorm; // Single channel 8-bit
    case GrayscaleAlpha:
        if (image_info.bit_depth == 32) {
            return wis::DataFormat::RG32Float; // Two channels 32-bit float
        } else if (image_info.bit_depth == 16) {
            return wis::DataFormat::RG16Unorm; // Two channels 16-bit
        }
        return wis::DataFormat::RG8Unorm; // Two channels 8-bit
    case RGBA:
        if (image_info.bit_depth == 32) {
            return wis::DataFormat::RGBA32Float; // Four channels 32-bit float
        } else if (image_info.bit_depth == 16) {
            return wis::DataFormat::RGBA16Unorm; // Four channels 16-bit
        }
        return wis::DataFormat::RGBA8Unorm; // Four channels 8-bit
    default:
        break;
    }
    return wis::DataFormat::Unknown; // Fallback for unsupported formats
}

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

    switch (channels) {
    case 1:
        image_info.color_components = vortex::codec::ColorComponents::Grayscale;
        break;
    case 2:
        image_info.color_components = vortex::codec::ColorComponents::GrayscaleAlpha;
        break;
    case 3:
        image_info.color_components = vortex::codec::ColorComponents::RGB;
        vortex::warn("STBCodec::LoadTexture: !DEV! RGB image format detected in file: {}, converting to RGBA.", file_path.string());
        break;
    case 4:
        image_info.color_components = vortex::codec::ColorComponents::RGBA;
        break;
    default:
        vortex::error("STBCodec::LoadTexture: Unsupported number of channels ({}) in file: {}", channels, file_path.string());
        return {};
    }

    if (!data) {
        // get the error message from stb_image
        const char* error_msg = stbi_failure_reason();
        vortex::error("STBCodec::LoadTexture: Failed to load image from file: {}. Error: {}", file_path.string(), error_msg);
        return {};
    }

    wis::DataFormat data_format = determine_format(image_info);
    if (data_format == wis::DataFormat::Unknown) {
        vortex::error("STBCodec::LoadTexture: !DEV! Unknown image format in file: {}, FIX!", file_path.string());
        return {};
    }

    image_info.width = width;
    image_info.height = height;
    image_info.channels = channels;

    wis::Result res = wis::success;
    auto& allocator = gfx.GetExtendedAllocation();
    wis::TextureDesc texture_desc{
        .format = data_format,
        .size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
        .mip_levels = 1,
        .usage = wis::TextureUsage::HostCopy | wis::TextureUsage::ShaderResource | wis::TextureUsage::CopySrc,
    };

    wis::Texture texture = allocator.CreateGPUUploadTexture(res, gfx.GetAllocator(), texture_desc);
    if (!success(res)) {
        vortex::error("STBCodec::LoadTexture: Failed to create texture: {}. Descriptor = {}", res.error, texture_desc);
        return {};
    }

    wis::TextureRegion region{
        .offset = { 0, 0, 0 },
        .size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
        .mip = 0,
        .array_layer = 0,
        .format = data_format,
    };
    res = allocator.WriteMemoryToSubresourceDirect(data.get(), texture, wis::TextureState::Common, region);
    if (!success(res)) {
        vortex::error("STBCodec::LoadTexture: Failed to write memory to subresource: {}. \nDescriptor = {}. \nRegion = {}", res.error, texture_desc, region);
        return {};
    }

    return texture;
}