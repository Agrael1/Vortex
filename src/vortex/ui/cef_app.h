#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <fstream>
#include <vortex/util/log.h>

namespace vortex::ui {
class VortexResourceHandler : public CefImplements<VortexResourceHandler, CefResourceHandler>
{
public:
    bool Open(CefRefPtr<CefRequest> request,
              bool& handle_request,
              CefRefPtr<CefCallback> callback) override
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
    void GetResponseHeaders(CefRefPtr<CefResponse> response,
                            int64_t& response_length,
                            CefString& redirectUrl) override
    {
        if (!_mime_type.empty()) {
            response->SetMimeType(_mime_type);
        }

        // Set response headers here
        response->SetStatus(200);
        response_length = _file_size; // Set the response length to the file size
    }
    bool Read(void* data_out,
              int bytes_to_read,
              int& bytes_read,
              CefRefPtr<CefResourceReadCallback> callback) override
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
    void Cancel() override
    {
        // Handle cancellation if needed
    }

private:
    std::ifstream _file_stream; ///< File stream for reading resource data
    uint64_t _file_size = 0; ///< Size of the resource file
    CefString _mime_type; ///< MIME type of the resource
};
class VortexSchemeHandlerFactory : public CefImplements<VortexSchemeHandlerFactory, CefSchemeHandlerFactory>
{
public:
    CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         const CefString& scheme_name,
                                         CefRefPtr<CefRequest> request) override
    {
        // Handle requests for the "vortex" scheme
        return new vortex::ui::VortexResourceHandler();
    }
};
class VortexV8Handler : public CefImplements<VortexV8Handler, CefV8Handler>
{
public:
    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) override
    {
        // Handle the JavaScript function call
        if (name == "vortexCall") {
            if (arguments.size() < 1) {
                exception = "Invalid number of arguments";
                return false;
            }
            // Process the function call here
            // For example, you can log the arguments or perform some action
            vortex::info("vortexCall executed with arguments: {}", arguments[0]->GetStringValue().ToString());

            retval = CefV8Value::CreateString("Hello From C++!");
            return true;
        }
        exception = "Unknown function: " + name.ToString();
        return false; // Function not found
    }

private:
    std::unordered_map<std::string, CefRefPtr<CefV8Value>> _registered_functions; ///< Map to store registered functions
};

class VortexCefApp : public CefImplements<VortexCefApp, CefApp, CefBrowserProcessHandler, CefRenderProcessHandler>
{
public:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
    {
        return this;
    }

public:
    void OnContextInitialized() override
    {
        CefRegisterSchemeHandlerFactory(u"http", u"vortex", new vortex::ui::VortexSchemeHandlerFactory());
    }

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override
    {
        // Register the JavaScript function
        CefRefPtr<CefV8Value> global = context->GetGlobal();
        CefRefPtr<VortexV8Handler> handler = new VortexV8Handler();
        CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("vortexCall", handler);
        global->SetValue("vortexCall", func, V8_PROPERTY_ATTRIBUTE_NONE);
    }
};
} // namespace vortex::ui