#pragma once
#include <vortex/ui/implements.h>
#include <vortex/ui/value.h>
#include <vortex/ui/call_handler.h>
#include <include/cef_app.h>
#include <fstream>
#include <include/wrapper/cef_helpers.h>

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

private:
    CefRefPtr<VortexV8Handler> _handler;
};
} // namespace vortex::ui