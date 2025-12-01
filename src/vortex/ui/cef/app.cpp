#include <vortex/ui/cef/app.h>
#include <include/cef_parser.h>

#include <array>
#include <system_error>

#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__linux__)
#  include <unistd.h>
#  include <limits.h>
#endif

namespace {

std::filesystem::path DetermineExecutableDirectory()
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = 0;
    while (true) {
        length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
    return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    std::array<char, PATH_MAX> buffer{};
    const auto length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }
    buffer[static_cast<size_t>(length)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
#else
    return {};
#endif
}

const std::filesystem::path& RuntimeRoot()
{
    static const std::filesystem::path root = [] {
        auto exe_dir = DetermineExecutableDirectory();
        if (exe_dir.empty()) {
            vortex::warn("VortexResourceHandler: Unable to determine executable directory, falling back to current_path()");
            exe_dir = std::filesystem::current_path();
        }
        return exe_dir;
    }();
    return root;
}

bool IsUnderRoot(const std::filesystem::path& root, const std::filesystem::path& candidate)
{
    auto root_str = root.lexically_normal().generic_string();
    auto candidate_str = candidate.lexically_normal().generic_string();
    if (root_str.empty()) {
        return true;
    }
    if (!root_str.empty() && root_str.back() != '/') {
        root_str.push_back('/');
    }
    if (candidate_str.size() < root_str.size()) {
        return false;
    }
    return candidate_str.rfind(root_str, 0) == 0;
}

std::filesystem::path DefaultUiEntry()
{
    return std::filesystem::path("ui") / "index.html";
}

} // namespace

bool vortex::ui::VortexResourceHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback)
{
    handle_request = true; // Indicate that this handler will handle the request

    CefURLParts parts;
    CefParseURL(request->GetURL(), parts);
    std::u16string_view raw_path(parts.path.str ? parts.path.str : u"", parts.path.length);
    std::filesystem::path relative_path;
    if (!raw_path.empty()) {
        relative_path = std::filesystem::path(std::u16string(raw_path)).relative_path();
    }
    if (relative_path.empty() || relative_path == ".") {
        relative_path = DefaultUiEntry();
    }
    auto resolved_path = (RuntimeRoot() / relative_path).lexically_normal();
    if (!IsUnderRoot(RuntimeRoot(), resolved_path)) {
        vortex::error("VortexResourceHandler::Open: Attempt to access resource outside runtime root: {}", resolved_path.string());
        return false;
    }
    if (!std::filesystem::exists(resolved_path)) {
        vortex::error("VortexResourceHandler::Open: Resource not found: {}", resolved_path.string());
        return false; // Resource not found
    }
    std::error_code size_error;
    _file_size = std::filesystem::file_size(resolved_path, size_error);
    if (size_error) {
        vortex::error("VortexResourceHandler::Open: Failed to read size of {}: {}", resolved_path.string(), size_error.message());
        return false;
    }
    auto ext = resolved_path.extension().string();
    // Remove leading dot from extension
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    auto mime = CefGetMimeType(ext);
    if (!mime.empty()) {
        _mime_type = mime.ToString();
    } else {
        if (ext == "js" || ext == "mjs") {
            _mime_type = "text/javascript";
        } else if (ext == "css") {
            _mime_type = "text/css";
        } else if (ext == "json") {
            _mime_type = "application/json";
        } else if (ext == "wasm") {
            _mime_type = "application/wasm";
        } else if (ext == "svg") {
            _mime_type = "image/svg+xml";
        } else if (ext == "png") {
            _mime_type = "image/png";
        } else if (ext == "jpg" || ext == "jpeg") {
            _mime_type = "image/jpeg";
        } else if (ext == "mp4") {
            _mime_type = "video/mp4";
        } else {
            _mime_type = "application/octet-stream";
        }
    }
    _file_stream.open(resolved_path, std::ios::binary);
    if (!_file_stream.is_open()) {
        vortex::error("VortexResourceHandler::Open: Failed to open resource file: {}", resolved_path.string());
        return false; // Failed to open the resource file
    }
    return true; // Successfully opened the resource
}

void vortex::ui::VortexResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl)
{
    if (!_mime_type.empty()) {
        response->SetMimeType(_mime_type);
    }

    // Set response headers here
    response->SetStatus(200);
    response_length = _file_size; // Set the response length to the file size
}

bool vortex::ui::VortexResourceHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback)
{
    if (!data_out) {
        vortex::error("VortexResourceHandler::Read: data_out is null");
        bytes_read = -2; // Indicate an error
        return false; // No data to read
    }

    _file_stream.read(static_cast<char*>(data_out), bytes_to_read);
    bytes_read = static_cast<int>(_file_stream.gcount());
    return bytes_read > 0; // Return true if data was read, false if no more data
}
