#include <vortex/codec/codec_ffmpeg.h>

// FFmpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

void vortex::codec::CodecFFmpeg::LoadTexture(const std::filesystem::path& path)
{
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, path.string().c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Could not open input file: " + path.string());
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        throw std::runtime_error("Could not find stream info for file: " + path.string());
    }


    //// Find the first video stream
    //AVCodec* codec = nullptr;
    //AVCodecParameters* codecParams = nullptr;
    //for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
    //    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    //        codecParams = formatContext->streams[i]->codecpar;
    //        codec = avcodec_find_decoder(codecParams->codec_id);
    //        break;
    //    }
    //}
    //if (!codec) {
    //    avformat_close_input(&formatContext);
    //    throw std::runtime_error("Could not find a suitable codec for file: " + path.string());
    //}
    //// Open the codec
    //AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    //if (!codecContext) {
    //    avformat_close_input(&formatContext);
    //    throw std::runtime_error("Could not allocate codec context for file: " + path.string());
    //}
    //if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
    //    avcodec_free_context(&codecContext);
    //    avformat_close_input(&formatContext);
    //    throw std::runtime_error("Could not copy codec parameters to context for file: " + path.string());
    //}
    //if (avcodec_open2(codecContext, codec, nullptr) < 0) {
    //    avcodec_free_context(&codecContext);
    //    avformat_close_input(&formatContext);
    //    throw std::runtime_error("Could not open codec for file: " + path.string());
    //}
    //// Read frames and process them as needed
    //// ...
    //// Clean up
    //avcodec_free_context(&codecContext);
    //avformat_close_input(&formatContext);
}
