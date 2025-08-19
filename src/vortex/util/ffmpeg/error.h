#pragma once
#include <system_error>
#include <string>

extern "C" {
#include <libavutil/error.h>
}

namespace vortex::ffmpeg {

// FFmpeg error category for std::error_code
class ffmpeg_error_category : public std::error_category {
public:
    const char* name() const noexcept override {
        return "ffmpeg";
    }
    
    std::string message(int ev) const override {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        if (av_strerror(ev, errbuf, sizeof(errbuf)) == 0) {
            return std::string(errbuf);
        }
        return "Unknown FFmpeg error";
    }
    
    std::error_condition default_error_condition(int ev) const noexcept override {
        // Map common FFmpeg errors to standard error conditions
        switch (ev) {
            case AVERROR_EOF:
                return std::errc::broken_pipe; // End of stream
            case AVERROR(ENOMEM):
                return std::errc::not_enough_memory;
            case AVERROR(EINVAL):
                return std::errc::invalid_argument;
            case AVERROR(ENOENT):
                return std::errc::no_such_file_or_directory;
            case AVERROR(EACCES):
                return std::errc::permission_denied;
            case AVERROR(EIO):
                return std::errc::io_error;
            default:
                return std::error_condition(ev, *this);
        }
    }
};

// Get the FFmpeg error category instance
inline const std::error_category& ffmpeg_category() {
    static ffmpeg_error_category instance;
    return instance;
}

// Create error_code from FFmpeg error
inline std::error_code make_ffmpeg_error(int ffmpeg_error_code) {
    return std::error_code(ffmpeg_error_code, ffmpeg_category());
}

// Helper enum for common FFmpeg errors (optional, for convenience)
enum class ffmpeg_errc {
    success = 0,
    end_of_file = AVERROR_EOF,
    out_of_memory = AVERROR(ENOMEM),
    invalid_data = AVERROR_INVALIDDATA,
    stream_not_found = AVERROR_STREAM_NOT_FOUND,
    decoder_not_found = AVERROR_DECODER_NOT_FOUND,
    // Add more as needed
};

// Make ffmpeg_errc work with std::error_code
inline std::error_code make_error_code(ffmpeg_errc e) {
    return std::error_code(static_cast<int>(e), ffmpeg_category());
}

} // namespace vortex::ffmpeg

// Register our enum for std::error_code
template<>
struct std::is_error_code_enum<vortex::ffmpeg::ffmpeg_errc> : std::true_type {};