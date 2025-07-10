#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_client.h>

namespace vortex::ui {
class Client : public CefImplements<Client, CefClient, CefLifeSpanHandler, CefDisplayHandler>
{
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        _browser = browser;
    }

public:
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
        return this; // No lifespan handling needed for this client
    }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
        return this; // Return this client as the display handler
    }
    CefBrowser* GetBrowser() noexcept
    {
        return _browser.get();
    }

        // Override OnConsoleMessage to redirect CEF console output to cout
    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                          cef_log_severity_t level,
                          const CefString& message,
                          const CefString& source,
                          int line) override
    {
        // Map CEF log levels to descriptive prefixes
        std::string message_str = message.ToString();

        switch (level) {
        case LOGSEVERITY_DEBUG:
            vortex::debug("CEF Console: {}", message_str);
            break;
        case LOGSEVERITY_INFO:
            vortex::info("CEF Console: {}", message_str);
            break;
        case LOGSEVERITY_WARNING:
            vortex::warn("CEF Console: {}", message_str);
            break;
        case LOGSEVERITY_ERROR:
            vortex::error("CEF Console: {}", message_str);
            break;
        case LOGSEVERITY_FATAL:
            vortex::critical("CEF Console: {}", message_str);
            break;
        }
        return true; // Return true to suppress the default console output
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override
    {


        vortex::info("Client::OnProcessMessageReceived: Received message from process {}: {}", reflect::enum_name(source_process), message->GetName().ToString());

        if (message->GetName() == "greetAsync") {
            // Handle the message here, e.g., call a function in the browser context
            auto a = CefProcessMessage::Create("co_return");
            auto args = a->GetArgumentList();
            args->SetSize(1); // Set size to 1 for the counter
            args->SetInt(0, counter++); // Increment the counter and send it back
            _browser->GetMainFrame()->SendProcessMessage(PID_BROWSER, a);
            return true; // Indicate that the message was handled
        }

        return false;
    }

private:
    CefRefPtr<CefBrowser> _browser;
    uint32_t counter = 57; ///< Example counter for demonstration purposes
};

} // namespace vortex::ui
