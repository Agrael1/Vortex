#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <fstream>
#include <vortex/util/log.h>

namespace vortex::ui {
class Property
{
public:
    Property(const std::string& initialValue = "")
        : value_(initialValue) { }

    std::string Get() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    void Set(const std::string& newValue)
    {
        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (value_ != newValue) {
                value_ = newValue;
                changed = true;
            }
        }

        if (changed && changeCallback_) {
            changeCallback_(newValue);
        }
    }

    void SetChangeCallback(std::function<void(const std::string&)> callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        changeCallback_ = callback;
    }

private:
    std::string value_;
    std::function<void(const std::string&)> changeCallback_;
    mutable std::mutex mutex_;
};

// Global property for button text
Property g_buttonText("Click Me!");

struct VTask : public CefImplements<VTask, CefTask> {
    VTask(std::function<void()> func)
        : _func(std::move(func)) { }
    void Execute() override
    {
        if (_func) {
            _func();
        }
    }
    std::function<void()> _func; ///< Function to execute when the task is run
};

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
        if (name == "getButtonText") {
            // Return the current button text
            retval = CefV8Value::CreateString(g_buttonText.Get());
            return true;
        } else if (name == "setButtonText") {
            // Set the button text from JavaScript
            if (arguments.size() < 1 || !arguments[0]->IsString()) {
                exception = "Expected string argument";
                return true;
            }

            std::string newText = arguments[0]->GetStringValue();
            g_buttonText.Set(newText);

            retval = CefV8Value::CreateBool(true);
            return true;
        } else if (name == "registerButtonTextCallback") {
            // Register a callback to be notified when button text changes
            if (arguments.size() < 1 || !arguments[0]->IsFunction()) {
                exception = "Expected function argument";
                return true;
            }

            // Store the callback and context
            buttonTextCallback_ = arguments[0];
            callbackContext_ = CefV8Context::GetCurrentContext();

            // Set up the C++ side callback
            g_buttonText.SetChangeCallback([this](const std::string& newValue) {
                NotifyJavaScript(newValue);
            });

            retval = CefV8Value::CreateBool(true);
            return true;
        }
        return false; // Function not found
    }

    // Notify JavaScript about property changes
    void NotifyJavaScript(const std::string& newValue)
    {
        if (buttonTextCallback_.get() && callbackContext_.get() && callbackContext_->IsValid()) {
            // Check if we're already on the correct thread
            if (callbackContext_->IsSame(CefV8Context::GetCurrentContext())) {
                // We're on the render thread with the right context, execute directly
                return ExecuteCallback(newValue);
            }
        }
        // Post to the render thread instead of UI thread
        CefPostTask(TID_RENDERER, new VTask([this, value = newValue]() {
                        if (callbackContext_.get() && callbackContext_->IsValid()) {
                            // Enter the context and execute
                            callbackContext_->Enter();
                            ExecuteCallback(value);
                            callbackContext_->Exit();
                        }
                    }));
    }
    void ExecuteCallback(const std::string& value)
    {
        if (buttonTextCallback_.get() && callbackContext_.get()) {
            // Create arguments for the callback
            CefV8ValueList args;
            args.push_back(CefV8Value::CreateString(value));

            // Call the JavaScript callback
            CefRefPtr<CefV8Value> result;
            CefRefPtr<CefV8Exception> exception;
            buttonTextCallback_->ExecuteFunction(nullptr, args);

            if (exception.get()) {
                // Log the exception if there was an error
                vortex::error("JavaScript callback error: {}", exception->GetMessage().ToString());
            }
        }
    }

private:
    CefRefPtr<CefV8Value> buttonTextCallback_;
    CefRefPtr<CefV8Context> callbackContext_;
};

std::string GetCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string timeStr = std::ctime(&time);
    // Remove newline
    if (!timeStr.empty() && timeStr[timeStr.length() - 1] == '\n') {
        timeStr.erase(timeStr.length() - 1);
    }
    return timeStr;
}

// Function to simulate changing the button text from C++
void SimulateButtonTextChange()
{
    // This could be called from another thread, timer, or event
    g_buttonText.Set("Updated from C++ at " + GetCurrentTimeString());
}

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


        global->SetValue("getButtonText",
                         CefV8Value::CreateFunction("getButtonText", handler),
                         V8_PROPERTY_ATTRIBUTE_NONE);

        global->SetValue("setButtonText",
                         CefV8Value::CreateFunction("setButtonText", handler),
                         V8_PROPERTY_ATTRIBUTE_NONE);

        global->SetValue("registerButtonTextCallback",
                         CefV8Value::CreateFunction("registerButtonTextCallback", handler),
                         V8_PROPERTY_ATTRIBUTE_NONE);
    }
};
} // namespace vortex::ui