#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/graphics.h>
#include <vortex/util/common.h>
#include <vortex/util/log.h>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

int save_frame_as_ppm(AVFrame* frame, const char* filename)
{
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return -1;
    }

    // Write PPM header
    fprintf(f, "P6\n%d %d\n255\n", frame->width, frame->height);

    // Write pixel data
    // Assuming frame is in RGB or RGBA format
    for (int y = 0; y < frame->height; y++) {
        for (int x = 0; x < frame->width; x++) {
            uint8_t* pixel = &frame->data[0][y * frame->linesize[0] + x * (frame->format == AV_PIX_FMT_RGBA ? 4 : 3)];
            // Write RGB values (skip alpha if present)
            fwrite(pixel, 1, 3, f);
        }
    }

    fclose(f);
    return 0;
}

vortex::Texture2D vortex::codec::CodecFFmpeg::LoadTexture(const Graphics& gfx, const std::filesystem::path& path)
{
    using unique_context = vortex::unique_any<AVFormatContext*, avformat_close_input>;
    using unique_codec_context = vortex::unique_any<AVCodecContext*, avcodec_free_context>;
    using unique_frame = vortex::unique_any<AVFrame*, av_frame_free>;
    using unique_packet = vortex::unique_any<AVPacket, av_packet_unref>;

    wis::Texture texture;

    auto path_str = path.string();
    if (!std::filesystem::exists(path)) {
        vortex::error("CodecFFmpeg::LoadTexture: File does not exist: {}", path_str);
        return vortex::Texture2D(std::move(texture)); // File does not exist, nothing to load
    }

    unique_context format_context;
    if (auto err = avformat_open_input(format_context.put(), path_str.c_str(), nullptr, nullptr); err < 0) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not open input file: {}. Error {}", path_str, err);
        return vortex::Texture2D(std::move(texture)); // Could not open the input file
    }

    if (auto err = avformat_find_stream_info(format_context.get(), nullptr); err < 0) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not find stream info for file: {}. Error {}", path_str, err);
        return vortex::Texture2D(std::move(texture)); // Could not find stream info
    }

    // Find the first video stream
    const AVCodec* codec = nullptr;
    AVCodecParameters* codec_params = nullptr;
    std::span<AVStream*> streams(format_context->streams, format_context->nb_streams);
    for (AVStream* stream : streams) {
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_params = stream->codecpar;
            codec = avcodec_find_decoder(codec_params->codec_id);
            if (codec) {
                break;
            }
        }
    }

    if (!codec) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not find a suitable codec for file: {}", path_str);
        return vortex::Texture2D(std::move(texture)); // No suitable codec found
    }

    // Open the codec
    unique_codec_context codec_context{ avcodec_alloc_context3(codec) };
    if (!codec_context) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not allocate codec context for file: {}", path_str);
        return vortex::Texture2D(std::move(texture)); // Could not allocate codec context
    }

    if (auto err = avcodec_parameters_to_context(codec_context.get(), codec_params); err < 0) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not copy codec parameters to context for file: {}. Error {}", path_str, err);
        return vortex::Texture2D(std::move(texture)); // Could not copy codec parameters to context
    }
    if (auto err = avcodec_open2(codec_context.get(), codec, nullptr); err < 0) {
        vortex::error("CodecFFmpeg::LoadTexture: Could not open codec for file: {}. Error {}", path_str, err);
        return vortex::Texture2D(std::move(texture)); // Could not open codec
    }

    // Read data from the stream
    unique_packet packet;
    while (av_read_frame(format_context.get(), packet.put()) >= 0) {
        if (packet->stream_index < 0 || packet->stream_index >= format_context->nb_streams) {
            continue; // Skip invalid stream index
        }
        // Decode the packet
        int response = avcodec_send_packet(codec_context.get(), &packet.get());
        if (response < 0) {
            vortex::error("CodecFFmpeg::LoadTexture: Error sending packet for decoding: {}", response);
            continue; // Error sending packet
        }

        unique_frame frame{ av_frame_alloc() };
        if (!frame) {
            vortex::error("CodecFFmpeg::LoadTexture: Could not allocate frame for decoding");
            continue; // Could not allocate frame
        }

        response = avcodec_receive_frame(codec_context.get(), frame.get());
        if (response < 0) {
            vortex::error("CodecFFmpeg::LoadTexture: Error receiving frame from decoder: {}", response);
            continue; // Error receiving frame
        }

        // Resample the frame if necessary
        if (frame->format != AV_PIX_FMT_RGBA) {
            // Create a SwsContext for resampling
            SwsContext* sws_ctx = sws_getContext(
                    frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                    frame->width, frame->height, AV_PIX_FMT_RGBA,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!sws_ctx) {
                vortex::error("CodecFFmpeg::LoadTexture: Could not create SwsContext for resampling");
                continue; // Could not create SwsContext
            }
            // Allocate a new frame for the resampled data
            unique_frame resampled_frame{ av_frame_alloc() };
            if (!resampled_frame) {
                vortex::error("CodecFFmpeg::LoadTexture: Could not allocate resampled frame");
                sws_freeContext(sws_ctx);
                continue; // Could not allocate resampled frame
            }
            resampled_frame->format = AV_PIX_FMT_RGBA;
            resampled_frame->width = frame->width;
            resampled_frame->height = frame->height;
            resampled_frame->linesize[0] = frame->width * 4; // RGBA has 4 bytes per pixel

            if (av_frame_get_buffer(resampled_frame.get(), 0) < 0) {
                vortex::error("CodecFFmpeg::LoadTexture: Could not allocate buffer for resampled frame");
                sws_freeContext(sws_ctx);
                continue; // Could not allocate buffer for resampled frame
            }
            // Perform the resampling
            int resample_response = sws_scale(
                    sws_ctx, frame->data, frame->linesize,
                    0, frame->height,
                    resampled_frame->data, resampled_frame->linesize);



            sws_freeContext(sws_ctx);
            if (resample_response < 0) {
                vortex::error("CodecFFmpeg::LoadTexture: Error resampling frame: {}", resample_response);
                continue; // Error resampling frame
            }
            // Use the resampled frame for further processing
            frame = std::move(resampled_frame);
        }

        // Process the decoded frame (e.g., convert to texture)
        wis::Result result = wis::success;
        auto& ext_alloc = gfx.GetExtendedAllocation();
        wis::TextureDesc desc{
            .format = wis::DataFormat::RGBA8Unorm,
            .size = { static_cast<uint32_t>(frame->width), static_cast<uint32_t>(frame->height) },
            .usage = wis::TextureUsage::HostCopy | wis::TextureUsage::ShaderResource
        };
        texture = ext_alloc.CreateGPUUploadTexture(result, gfx.GetAllocator(), desc);

        if (!success(result)) {
            vortex::error("CodecFFmpeg::LoadTexture: Failed to create texture for file: {}.\nError {}.\nTexture {}", path_str, result.error, desc);
            continue; // Failed to create texture
        }

        // Copy the frame data to the texture
        wis::TextureRegion frame_region{
            .offset = { 0, 0, 0 },
            .size = { static_cast<uint32_t>(frame->width), static_cast<uint32_t>(frame->height), 1 },
            .mip = 0,
            .array_layer = 0,
            .format = wis::DataFormat::RGBA8Unorm
        };
        result = ext_alloc.WriteMemoryToSubresourceDirect(frame->data[0], texture, wis::TextureState::Common, frame_region);
        if (!success(result)) {
            vortex::error("CodecFFmpeg::LoadTexture: Failed to write memory to subresource for file: {}.\nError {}.\nTexture {}.\nRegion {}", path_str, result.error, desc, frame_region);
            continue; // Failed to write memory to subresource
        }
        // Successfully loaded the texture
        return vortex::Texture2D(std::move(texture), wis::Size2D{ desc.size.width, desc.size.height }, desc.format);
    }
    return vortex::Texture2D(std::move(texture)); // Return the texture if no frames were processed
}
