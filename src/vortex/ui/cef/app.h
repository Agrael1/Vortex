#pragma once
#include <vortex/ui/implements.h>
#include <vortex/ui/value.h>
#include <vortex/ui/call_handler.h>
#include <vortex/util/lib/reflect.h>

#include <fstream>

#include <include/cef_app.h>
#include <include/cef_api_versions.h>

namespace vortex::ui {
class VortexResourceHandler : public CefImplements<VortexResourceHandler, CefResourceHandler>
{
public:
    bool Open(CefRefPtr<CefRequest> request,
              bool& handle_request,
              CefRefPtr<CefCallback> callback) override;

    void GetResponseHeaders(CefRefPtr<CefResponse> response,
                            int64_t& response_length,
                            CefString& redirectUrl) override;
    bool Read(void* data_out,
              int bytes_to_read,
              int& bytes_read,
              CefRefPtr<CefResourceReadCallback> callback) override;
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
class VortexCefApp : public CefImplements<VortexCefApp, CefApp, CefBrowserProcessHandler, CefRenderProcessHandler>
{
    struct InitGuard
    {
        explicit InitGuard(bool init)
            : init(init)
        {
        }
        ~InitGuard()
        {
            if (init) CefShutdown();
        }
        operator bool() const noexcept
        {
            return init;
        }

    private:
        bool init = false;
    };

public:
    VortexCefApp([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
#ifdef _WIN32 
      : _main_args(GetModuleHandle(nullptr))
#else
      : _main_args(argc, argv)
#endif
    {

    }

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
        _handler = new vortex::ui::VortexV8Handler();
    }
    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) override
    {
        vortex::info("VortexCefApp::OnProcessMessageReceived: Received message from process {}: {}", reflect::enum_name(source_process), message->GetName().ToString());
        if (message->GetName() == "co_return") {
            // Handle the message using the VortexV8Handler
            _handler->ResolvePromise(message->GetArgumentList());
            return true; // Message handled
        }
        return false; // Message not handled
    }
    const CefMainArgs& GetMainArgs() const noexcept
    {
        return _main_args;
    }

    int StartCefSubprocess() noexcept
    {
        return CefExecuteProcess(_main_args, this, nullptr);
    }
    InitGuard InitializeCef() noexcept
    {
        CefSettings cef_settings;
        cef_settings.multi_threaded_message_loop = true;
        cef_settings.no_sandbox = true;
        CefString(&cef_settings.cache_path).FromString((std::filesystem::current_path() / std::format("cef_cache_{}", CEF_API_VERSION_LAST)).string()); // Set cache path
        
        if (!CefInitialize(_main_args, cef_settings, this, nullptr)) {
            int exit_code = CefGetExitCode();
            vortex::critical("CefInitialize failed with error {}: {}", exit_code, 
                reflect::enum_name(cef_return_value_t(exit_code)));
            return InitGuard{ false }; // Initialization failed
        }
        return InitGuard{ true }; // Successfully initialized
    }

private:
    CefRefPtr<VortexV8Handler> _handler;
    CefMainArgs _main_args;
};
} // namespace vortex::ui