#include <vortex/codec/codec_png.h>
#include <vortex/codec/codec_common.h>
#include <vortex/graphics.h>
#include <util/log.h>
#include <png.h>
using namespace vortex::codec;

void vortex::codec::PNGCodec::ErrorHandler(png_structp png_ptr, png_const_charp error_msg)
{
    // Handle PNG errors by logging them
    vortex::error("PNGCodec Error: {}", error_msg);
    longjmp(png_jmpbuf(png_ptr), 1); // Jump to the setjmp point
}
void vortex::codec::PNGCodec::WarningHandler(png_structp png_ptr, png_const_charp warning_msg)
{
    // Handle PNG warnings by logging them
    vortex::warn("PNGCodec Warning: {}", warning_msg);
}

struct cICPChunk {
    uint8_t primaries;
    uint8_t transfer_function;
    uint8_t matrix_coeff;
    uint8_t video_full_range;
};

struct PNGReader {
    static constexpr auto header_size = 8; // PNG header size
    using PNGFile = vortex::unique_file;

    struct PNGBinder {
        png_struct* png_ptr = nullptr;
        png_info* info_ptr = nullptr;

        ~PNGBinder()
        {
            Reset();
        }

    public:
        operator bool() const noexcept
        {
            return png_ptr && info_ptr;
        }
        void Reset()
        {
            if (png_ptr) {
                png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            }
            png_ptr = nullptr;
            info_ptr = nullptr;
        }
    };

public:
    PNGReader(const std::filesystem::path& file_path)
        : file_ptr(LoadFile(file_path))
        , png_binder(CreatePngBinder())
    {
        if (!file_ptr || !png_binder) {
            return;
        }

        auto* png_ptr = png_binder.png_ptr;
        auto* info_ptr = png_binder.info_ptr;

        if (setjmp(png_jmpbuf(png_ptr))) {
            vortex::error("PNGCodec::LoadTexture: Error during PNG file reading for {}", file_path.string());
            Reset();
            return;
        }

        png_init_io(png_ptr, file_ptr.get());
        png_set_sig_bytes(png_ptr, header_size);
        png_read_info(png_ptr, info_ptr);

        // Get image info
        info = CreateInfo();
    }

public:
    operator bool() const noexcept
    {
        return file_ptr && bool(png_binder);
    }
    ImageInfo GetInfo() const noexcept
    {
        return info;
    }
    bool ReadImageData(unsigned char* image_data) const
    {
        if (!file_ptr || !png_binder) {
            vortex::error("PNGReader::ReadImageData: File pointer or PNG binder is null");
            return false;
        }
        auto* png_ptr = png_binder.png_ptr;
        auto* info_ptr = png_binder.info_ptr;
        png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * info.height);

        // Allocate row pointers
        if (!row_pointers) {
            vortex::error("Error: Could not allocate memory for row pointers\n");
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            vortex::error("PNGCodec::ReadImageData: Error during PNG file reading");
            free(row_pointers);
            return false;
        }

        // Set up row pointers
        for (int y = 0; y < info.height; y++) {
            row_pointers[y] = image_data + y * info.row_stride;
        }

        png_read_image(png_ptr, row_pointers);
        free(row_pointers);
        png_read_end(png_ptr, info_ptr); // Finalize reading
        return true;
    }
    cICPChunk GetCICP() const noexcept
    {
        if (!file_ptr || !png_binder) {
            vortex::error("PNGReader::GetCICP: File pointer or PNG binder is null");
            return {};
        }
        auto* png_ptr = png_binder.png_ptr;
        auto* info_ptr = png_binder.info_ptr;
        cICPChunk cicp{};
        png_get_cICP(png_ptr, info_ptr, &cicp.primaries, &cicp.transfer_function, &cicp.matrix_coeff, &cicp.video_full_range);
        return cicp;
    }

private:
    PNGFile LoadFile(const std::filesystem::path& file_path)
    {
        std::string file_path_str = file_path.string();
        if (!std::filesystem::exists(file_path)) {
            vortex::error("PNGReader::LoadFile: File does not exist: {}", file_path_str);
            return {};
        }

        FILE* file = fopen(file_path_str.c_str(), "rb");
        if (!file) {
            vortex::error("PNGReader::LoadFile: Could not open file: {}", file_path_str);
            return {};
        }
        unsigned char header[header_size + 1];
        fread(header, 1, header_size, file);
        if (png_sig_cmp(header, 0, header_size)) {
            vortex::error("PNGCodec::LoadTexture: {} is not a valid PNG file", file_path_str);
            return {};
        }

        return file;
    }
    PNGBinder CreatePngBinder()
    {
        PNGBinder binder;
        if (!file_ptr) {
            vortex::error("PNGReader::CreatePngBinder: File pointer is null");
            return binder;
        }

        binder.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, vortex::codec::PNGCodec::ErrorHandler, vortex::codec::PNGCodec::WarningHandler);
        if (!binder.png_ptr) {
            vortex::error("PNGReader::CreatePngReadStruct: png_create_read_struct failed");
            return binder;
        }
        binder.info_ptr = png_create_info_struct(binder.png_ptr);
        if (!binder.info_ptr) {
            vortex::error("PNGReader::CreatePngInfoStruct: png_create_info_struct failed");
        }
        return binder;
    }
    ImageInfo CreateInfo()
    {
        auto* png_ptr = png_binder.png_ptr;
        auto* info_ptr = png_binder.info_ptr;
        if (setjmp(png_jmpbuf(png_ptr))) {
            vortex::error("PNGCodec::LoadTexture: Error during PNG file info");
            Reset();
            return {};
        }

        ImageInfo info{};
        info.width = png_get_image_width(png_ptr, info_ptr);
        info.height = png_get_image_height(png_ptr, info_ptr);
        info.bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_byte color_type = png_get_color_type(png_ptr, info_ptr);
        switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
            info.channels = 1;
            info.color_components = ColorComponents::Grayscale;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            info.channels = 2;
            info.color_components = ColorComponents::GrayscaleAlpha;
            break;
        case PNG_COLOR_TYPE_PALETTE:
            // Convert palette to RGB
            png_set_palette_to_rgb(png_ptr);
            [[fallthrough]];
        case PNG_COLOR_TYPE_RGB:
            // Transform RGB to RGBA, since DX12 prefers RGBA textures
            png_set_filler(png_ptr, 0xFFFFFFFF, PNG_FILLER_AFTER); // Add alpha channel with value 255
            png_read_update_info(png_ptr, info_ptr);
            [[fallthrough]];
        case PNG_COLOR_TYPE_RGB_ALPHA:
            info.channels = 4;
            info.color_components = ColorComponents::RGBA;
            break;
        default:
            vortex::error("PNGReader::GetInfo: Unsupported color type: {}", color_type);
            return {};
        }

        // Handle transparency
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr);
            info.channels++;
        }

        // Update the PNG info after our transformations
        png_read_update_info(png_ptr, info_ptr);
        info.row_stride = png_get_rowbytes(png_ptr, info_ptr);
        return info;
    }
    void Reset()
    {
        file_ptr.reset({});
        png_binder.Reset();
    }

public:
    PNGFile file_ptr{};
    PNGBinder png_binder;
    ImageInfo info{ 0, 0, 0, 0, 0, ColorComponents::RGB }; // Default to RGB
};

wis::Texture vortex::codec::PNGCodec::LoadTexture(const vortex::Graphics& gfx, const std::filesystem::path& file_path)
{
    wis::Result result = wis::success;
    const wis::ExtendedAllocation& alloc = gfx.GetExtendedAllocation();
    PNGReader reader{ file_path };
    if (!reader) {
        return {}; // Return an empty texture
    }

    ImageInfo info = reader.GetInfo();
    cICPChunk cicp = reader.GetCICP();

    wis::TextureDesc desc{
        .format = wis::DataFormat::RGBA8Unorm, // Default format, TODO: Handle different formats
        .size = { info.width, info.height, 1 },
        .usage = wis::TextureUsage::ShaderResource | wis::TextureUsage::HostCopy | wis::TextureUsage::CopySrc,
    };
    wis::Texture texture = alloc.CreateGPUUploadTexture(result, gfx.GetAllocator(), desc);
    vortex::info("PNGCodec::LoadTexture: Creating GPU upload texture for {}", desc);
    if (!success(result)) {
        vortex::error("PNGCodec::LoadTexture: Failed to create GPU upload texture for {}", desc);
        return {};
    }

    unsigned char* image_data = (unsigned char*)malloc(info.row_stride * info.height);
    if (!image_data) {
        vortex::error("PNGCodec::LoadTexture: Could not allocate memory for image data\n");
        return {};
    }
    if (!reader.ReadImageData(image_data)) {
        vortex::error("PNGCodec::LoadTexture: Failed to read image data from PNG file");
        free(image_data);
        return {};
    }

    wis::TextureRegion region{
        .size = { info.width, info.height, 1 },
        .format = desc.format,
    };
    result = alloc.WriteMemoryToSubresourceDirect(image_data, texture, wis::TextureState::Common, region);
    free(image_data);
    if (!success(result)) {
        vortex::error("PNGCodec::LoadTexture: Failed to write memory region {} to texture {}", region, desc);
        return {};
    }

    return texture;
}
