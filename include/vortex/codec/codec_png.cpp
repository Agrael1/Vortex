#include <vortex/codec/codec_png.h>
#include <vortex/graphics.h>
#include <util/log.h>
#include <png.h>

// Function to load a PNG file, preserving HDR data if present
unsigned char* load_hdr_png(const char* filename, int* width, int* height,
                            int* channels, int* bit_depth)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return NULL;
    }

    // Check PNG signature
    unsigned char header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        fprintf(stderr, "Error: %s is not a valid PNG file\n", filename);
        fclose(fp);
        return NULL;
    }

    // Initialize PNG read structure
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error: png_create_read_struct failed\n");
        fclose(fp);
        return NULL;
    }

    // Initialize PNG info structure
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Error: png_create_info_struct failed\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    // Setup error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during PNG file reading\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    // Initialize PNG I/O
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    // Read PNG info
    png_read_info(png_ptr, info_ptr);

    // Get image info
    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    *bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);

    // Determine number of channels
    switch (color_type) {
    case PNG_COLOR_TYPE_GRAY:
        *channels = 1;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        *channels = 2;
        break;
    case PNG_COLOR_TYPE_RGB:
        *channels = 3;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        *channels = 4;
        break;
    default:
        // Handle palette images by converting to RGB
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
            *channels = 3;
        } else {
            fprintf(stderr, "Unsupported color type\n");
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return NULL;
        }
    }

    // For HDR images, we want to keep the 16-bit depth
    // For non-HDR images, we might want to convert to 8-bit
    // Uncomment the following line to force 8-bit conversion:
    // if (*bit_depth == 16) png_set_strip_16(png_ptr);

    // Handle transparency
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
        (*channels)++;
    }

    // Update the PNG info after our transformations
    png_read_update_info(png_ptr, info_ptr);

    // Calculate row bytes
    png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    // Allocate memory for the image data
    // For 16-bit images, we need twice as much memory per pixel
    unsigned char* image_data = (unsigned char*)malloc(row_bytes * (*height));
    if (!image_data) {
        fprintf(stderr, "Error: Could not allocate memory for image data\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    // Allocate row pointers
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * (*height));
    if (!row_pointers) {
        fprintf(stderr, "Error: Could not allocate memory for row pointers\n");
        free(image_data);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    // Set up row pointers
    for (int y = 0; y < *height; y++) {
        row_pointers[y] = image_data + y * row_bytes;
    }

    // Read the image data
    png_read_image(png_ptr, row_pointers);

    // Clean up
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    return image_data;
}

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

enum class PNGColorType {
    Grayscale = PNG_COLOR_TYPE_GRAY,
    GrayscaleAlpha = PNG_COLOR_TYPE_GRAY_ALPHA,
    RGB = PNG_COLOR_TYPE_RGB,
    RGBA = PNG_COLOR_TYPE_RGB_ALPHA,
    Palette = PNG_COLOR_TYPE_PALETTE
};

struct PNGInfo {
    int width;
    int height;
    int bit_depth;
    int channels;
    PNGColorType color_type;
};

struct PNGReader {
    static constexpr auto header_size = 8; // PNG header size
    using PngFile = std::unique_ptr<FILE, decltype(&fclose)>;
    using PngReadStruct = std::unique_ptr<png_struct, decltype(&png_destroy_read_struct)>;
    using PngInfo = std::unique_ptr<png_info, decltype(&png_destroy_info_struct)>;

public:
    PNGReader(const std::filesystem::path& file_path)
        : file_ptr(LoadFile(file_path))
        , png_ptr(CreatePngReadStruct())
        , info_ptr(CreatePngInfoStruct(png_ptr.get()))
    {
        if (setjmp(png_jmpbuf(png_ptr.get()))) {
            vortex::error("PNGCodec::LoadTexture: Error during PNG file reading for {}", file_path);
            Reset();
            return;
        }

        png_init_io(png_ptr.get(), file_ptr.get());
        png_set_sig_bytes(png_ptr.get(), header_size);
        png_read_info(png_ptr.get(), info_ptr.get());

        // Get image info
        info = GetInfo();
    }

public:
    operator bool() const noexcept
    {
        return file_ptr && png_ptr && info_ptr;
    }
    PNGInfo GetInfo() const noexcept
    {
        return info;
    }

private:
    PngFile LoadFile(const std::filesystem::path& file_path)
    {
        FILE* file = fopen(file_path.string().c_str(), "rb");
        if (!file) {
            vortex::error("PNGReader::LoadFile: Could not open file: {}", file_path.string());
            return PngFile(nullptr, fclose);
        }
        unsigned char header[header_size];
        fread(header, 1, header_size, file_ptr.get());
        if (png_sig_cmp(header, 0, header_size)) {
            vortex::error("PNGCodec::LoadTexture: {} is not a valid PNG file", file_path);
            return PngFile(nullptr, fclose);
        }

        return PngFile(file, fclose);
    }
    PngReadStruct CreatePngReadStruct()
    {
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, vortex::codec::PNGCodec::ErrorHandler, vortex::codec::PNGCodec::WarningHandler);
        if (!png_ptr) {
            vortex::error("PNGReader::CreatePngReadStruct: png_create_read_struct failed");
            return PngReadStruct(nullptr, png_destroy_read_struct);
        }
        return PngReadStruct(png_ptr, png_destroy_read_struct);
    }
    PngInfo CreatePngInfoStruct(png_structp png_ptr)
    {
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            vortex::error("PNGReader::CreatePngInfoStruct: png_create_info_struct failed");
            return PngInfo(nullptr, png_destroy_info_struct);
        }
        return PngInfo(info_ptr, png_destroy_info_struct);
    }
    PNGInfo CreateInfo()
    {
        if (setjmp(png_jmpbuf(png_ptr.get()))) {
            vortex::error("PNGCodec::LoadTexture: Error during PNG file info");
            Reset();
            return;
        }

        PNGInfo info{};
        info.width = png_get_image_width(png_ptr.get(), info_ptr.get());
        info.height = png_get_image_height(png_ptr.get(), info_ptr.get());
        info.bit_depth = png_get_bit_depth(png_ptr.get(), info_ptr.get());
        png_byte color_type = png_get_color_type(png_ptr.get(), info_ptr.get());
        switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
            info.channels = 1;
            info.color_type = PNGColorType::Grayscale;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            info.channels = 2;
            info.color_type = PNGColorType::GrayscaleAlpha;
            break;
        case PNG_COLOR_TYPE_RGB:
            info.channels = 3;
            info.color_type = PNGColorType::RGB;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            info.channels = 4;
            info.color_type = PNGColorType::RGBA;
            break;
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(png_ptr.get());
            info.channels = 3; // After conversion, it becomes RGB
            info.color_type = PNGColorType::RGB;
            break;
        default:
            vortex::error("PNGReader::GetInfo: Unsupported color type: {}", color_type);
            return {};
        }

        // Handle transparency
        if (png_get_valid(png_ptr.get(), info_ptr.get(), PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr.get());
            info.channels++;
        }

        return info;
    }
    void Reset()
    {
        file_ptr.reset();
        png_ptr.reset();
        info_ptr.reset();
    }

public:
    PngFile file_ptr{ nullptr, fclose };
    PngReadStruct png_ptr{ nullptr, png_destroy_read_struct };
    PngInfo info_ptr{ nullptr, png_destroy_info_struct };
    PNGInfo info{ 0, 0, 0, 0, PNGColorType::RGB }; // Default to RGB
};

wis::Texture vortex::codec::PNGCodec::LoadTexture(const vortex::Graphics& gfx, const std::filesystem::path& file_path)
{
    const wis::ExtendedAllocation& alloc = gfx.GetExtendedAllocation();
    PNGReader reader{ file_path };
    if (!reader) {
        return {}; // Return an empty texture
    }

    PNGInfo info = reader.GetInfo();
}
