#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/graphics.h>
#include <vortex/util/common.h>
#include <vortex/util/log.h>
#include <vortex/util/ffmpeg/error.h>



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

std::expected<vortex::Texture2D, std::error_code> vortex::codec::CodecFFmpeg::LoadTexture(const Graphics& gfx, const std::filesystem::path& path)
{
    using namespace vortex::ffmpeg;
    
    if (!std::filesystem::exists(path)) {
        auto ec = std::make_error_code(std::errc::no_such_file_or_directory);
        vortex::error("CodecFFmpeg::LoadTextureModern: File does not exist: {}", path.string());
        return std::unexpected(ec);
    }

    // Open input file

    auto connection_result = ConnectToStream(path.string());
    if (!connection_result) {
        vortex::error("CodecFFmpeg::LoadTextureModern: Failed to connect to stream: {}", connection_result.error().message());
        return std::unexpected(connection_result.error());
    }

    auto format_context = std::move(connection_result.value());

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
        auto ec = make_error_code(ffmpeg_errc::decoder_not_found);
        vortex::error("CodecFFmpeg::LoadTextureModern: Could not find a suitable codec for file: {}", path.string());
        return std::unexpected(ec);
    }

    // Open the codec
    unique_codec_context codec_context{ avcodec_alloc_context3(codec) };
    if (!codec_context) {
        auto ec = std::make_error_code(std::errc::not_enough_memory);
        vortex::error("CodecFFmpeg::LoadTextureModern: Could not allocate codec context for file: {}", path.string());
        return std::unexpected(ec);
    }

    int ret = avcodec_parameters_to_context(codec_context.get(), codec_params);
    if (ret < 0) {
        auto ec = make_ffmpeg_error(ret);
        vortex::error("CodecFFmpeg::LoadTextureModern: Could not copy codec parameters to context for file: {}. Error: {}", 
                     path.string(), ec.message());
        return std::unexpected(ec);
    }

    ret = avcodec_open2(codec_context.get(), codec, nullptr);
    if (ret < 0) {
        auto ec = make_ffmpeg_error(ret);
        vortex::error("CodecFFmpeg::LoadTextureModern: Could not open codec for file: {}. Error: {}", 
                     path.string(), ec.message());
        return std::unexpected(ec);
    }

    // Read and decode frames
    unique_packet packet;
    while (av_read_frame(format_context.get(), packet.put<clear_strategy::none>()) >= 0) {
        if (packet->stream_index < 0 || packet->stream_index >= format_context->nb_streams) {
            continue; // Skip invalid stream index
        }

        // Decode the packet
        ret = avcodec_send_packet(codec_context.get(), &packet.get());
        if (ret < 0) {
            auto ec = make_ffmpeg_error(ret);
            vortex::error("CodecFFmpeg::LoadTextureModern: Error sending packet for decoding: {}", ec.message());
            continue; // Try next packet
        }

        unique_frame frame{ av_frame_alloc() };
        if (!frame) {
            auto ec = std::make_error_code(std::errc::not_enough_memory);
            vortex::error("CodecFFmpeg::LoadTextureModern: Could not allocate frame for decoding");
            return std::unexpected(ec);
        }

        ret = avcodec_receive_frame(codec_context.get(), frame.get());
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                continue; // Need more input
            }
            auto ec = make_ffmpeg_error(ret);
            vortex::error("CodecFFmpeg::LoadTextureModern: Error receiving frame from decoder: {}", ec.message());
            continue; // Try next packet
        }

        // Convert frame to RGBA if necessary
        unique_frame final_frame = std::move(frame);
        if (final_frame->format != AV_PIX_FMT_RGBA) {
            // Create a SwsContext for resampling
            unique_swscontext sws_ctx{ sws_getContext(
                    final_frame->width, final_frame->height, static_cast<AVPixelFormat>(final_frame->format),
                    final_frame->width, final_frame->height, AV_PIX_FMT_RGBA,
                    SWS_BILINEAR, nullptr, nullptr, nullptr) };
            if (!sws_ctx) {
                auto ec = make_error_code(ffmpeg_errc::invalid_data);
                vortex::error("CodecFFmpeg::LoadTextureModern: Could not create SwsContext for resampling");
                return std::unexpected(ec);
            }

            // Allocate a new frame for the resampled data
            unique_frame resampled_frame{ av_frame_alloc() };
            if (!resampled_frame) {
                auto ec = std::make_error_code(std::errc::not_enough_memory);
                vortex::error("CodecFFmpeg::LoadTextureModern: Could not allocate resampled frame");
                return std::unexpected(ec);
            }

            resampled_frame->format = AV_PIX_FMT_RGBA;
            resampled_frame->width = final_frame->width;
            resampled_frame->height = final_frame->height;
            resampled_frame->linesize[0] = final_frame->width * 4; // RGBA has 4 bytes per pixel

            if (av_frame_get_buffer(resampled_frame.get(), 0) < 0) {
                auto ec = std::make_error_code(std::errc::not_enough_memory);
                vortex::error("CodecFFmpeg::LoadTextureModern: Could not allocate buffer for resampled frame");
                return std::unexpected(ec);
            }

            // Perform the resampling
            ret = sws_scale(sws_ctx.get(), final_frame->data, final_frame->linesize,
                           0, final_frame->height,
                           resampled_frame->data, resampled_frame->linesize);

            if (ret < 0) {
                auto ec = make_ffmpeg_error(ret);
                vortex::error("CodecFFmpeg::LoadTextureModern: Error resampling frame: {}", ec.message());
                return std::unexpected(ec);
            }

            // Use the resampled frame for further processing
            final_frame = std::move(resampled_frame);
        }

        // Create GPU texture
        wis::Result result = wis::success;
        auto& ext_alloc = gfx.GetExtendedAllocation();
        wis::TextureDesc desc{
            .format = wis::DataFormat::RGBA8Unorm,
            .size = { static_cast<uint32_t>(final_frame->width), static_cast<uint32_t>(final_frame->height) },
            .usage = wis::TextureUsage::HostCopy | wis::TextureUsage::ShaderResource
        };
        
        wis::Texture texture = ext_alloc.CreateGPUUploadTexture(result, gfx.GetAllocator(), desc);
        if (!success(result)) {
            // Map wis::Result to std::error_code (you may want to create a specific mapping function)
            auto ec = std::make_error_code(std::errc::not_enough_memory); // Simplified mapping
            vortex::error("CodecFFmpeg::LoadTextureModern: Failed to create texture for file: {}. Error: {}", 
                         path.string(), result.error);
            return std::unexpected(ec);
        }

        // Copy frame data to texture
        wis::TextureRegion frame_region{
            .offset = { 0, 0, 0 },
            .size = { static_cast<uint32_t>(final_frame->width), static_cast<uint32_t>(final_frame->height), 1 },
            .mip = 0,
            .array_layer = 0,
            .format = wis::DataFormat::RGBA8Unorm
        };
        
        result = ext_alloc.WriteMemoryToSubresourceDirect(final_frame->data[0], texture, wis::TextureState::Common, frame_region);
        if (!success(result)) {
            auto ec = std::make_error_code(std::errc::io_error); // Simplified mapping
            vortex::error("CodecFFmpeg::LoadTextureModern: Failed to write memory to subresource for file: {}. Error: {}", 
                         path.string(), result.error);
            return std::unexpected(ec);
        }

        // Successfully loaded the texture
        return vortex::Texture2D(std::move(texture), wis::Size2D{ desc.size.width, desc.size.height }, desc.format);
    }

    // No frames were processed
    auto ec = make_error_code(ffmpeg_errc::end_of_file);
    vortex::error("CodecFFmpeg::LoadTextureModern: No frames decoded from file: {}", path.string());
    return std::unexpected(ec);
}

std::expected<vortex::ffmpeg::unique_context, std::error_code>
vortex::codec::CodecFFmpeg::ConnectToStream(std::string_view stream_url)
{
    ffmpeg::unique_context format_context;
    int ret = avformat_open_input(format_context.address_of(), stream_url.data(), nullptr, nullptr);
    if (ret < 0) {
        auto ec = ffmpeg::make_ffmpeg_error(ret);
        vortex::error("CodecFFmpeg::ConnectToStream: Could not open input stream or file: {}. Error: {}",
                      stream_url, ec.message());
        return std::unexpected(ec);
    }
    ret = avformat_find_stream_info(format_context.get(), nullptr);
    if (ret < 0) {
        auto ec = ffmpeg::make_ffmpeg_error(ret);
        vortex::error("CodecFFmpeg::ConnectToStream: Could not find stream info for: {}. Error: {}",
                      stream_url, ec.message());
        return std::unexpected(ec);
    }
    return format_context;
}

std::expected<std::vector<vortex::codec::StreamInfo>, std::error_code>
vortex::codec::CodecFFmpeg::GetStreamInfo(const vortex::ffmpeg::unique_context& context)
{
    if (!context.is_valid()) {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    std::vector<StreamInfo> streams;
    auto* format_ctx = context.get();

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        AVStream* stream = format_ctx->streams[i];

        StreamInfo info;
        info.stream_index = i;

        // Get codec information
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (codec) {
            info.codec_name = codec->name;
        }

        // Media type
        switch (stream->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            info.width = stream->codecpar->width;
            info.height = stream->codecpar->height;

            // Convert AVRational to our ratio type
            auto av_to_ratio = [](AVRational av_rat) {
                return vortex::ratio64_t(av_rat.num, av_rat.den);
            };

            info.time_base = av_to_ratio(stream->time_base);
            info.frame_rate = av_to_ratio(stream->codecpar->framerate);
            info.avg_frame_rate = av_to_ratio(stream->avg_frame_rate);
            info.r_frame_rate = av_to_ratio(stream->r_frame_rate);
            break;

        case AVMEDIA_TYPE_AUDIO:
            info.sample_rate = stream->codecpar->sample_rate;
            info.channels = stream->codecpar->ch_layout.nb_channels;
            break;

        default:
            break;
        }

        info.start_time = stream->start_time;
        info.duration = stream->duration;

        streams.push_back(std::move(info));
    }

    return streams;
}

std::expected<vortex::codec::StreamInfo, std::error_code>
vortex::codec::CodecFFmpeg::FindBestVideoStream(const vortex::ffmpeg::unique_context& context)
{
    auto streams_result = GetStreamInfo(context);
    if (!streams_result) {
        return std::unexpected(streams_result.error());
    }

    // Find first video stream
    for (const auto& stream : *streams_result) {
        if (stream.media_type == "video") {
            return stream;
        }
    }

    return std::unexpected(vortex::ffmpeg::make_error_code(vortex::ffmpeg::ffmpeg_errc::stream_not_found));
}