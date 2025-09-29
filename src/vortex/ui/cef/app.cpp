#include <vortex/ui/cef/app.h>
#include <include/cef_parser.h>

bool vortex::ui::VortexResourceHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback)
{
    handle_request = true; // Indicate that this handler will handle the request

    CefURLParts parts;
    CefParseURL(request->GetURL(), parts);
    auto path = std::filesystem::path{ std::u16string_view(parts.path.str) }.relative_path();
    if (path.empty() || !std::filesystem::exists(path)) {
        vortex::error("VortexResourceHandler::Open: Resource not found: {}", path.string());
        return false; // Resource not found
    }
    _file_size = std::filesystem::file_size(path);
    auto ext = path.extension().string();
    // Remove leading dot from extension
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    _mime_type = CefGetMimeType(ext);
    _file_stream.open(path, std::ios::binary);
    if (!_file_stream.is_open()) {
        vortex::error("VortexResourceHandler::Open: Failed to open resource file: {}", path.string());
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
