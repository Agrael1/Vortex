#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_client.h>
#include <unordered_map>

namespace vortex::ui {
class Client : public CefImplements<Client, CefClient, CefLifeSpanHandler, CefDisplayHandler>
{
public:
    using MessageHandler = std::function<bool(CefRefPtr<CefProcessMessage>)>;
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

    void BindMessageHandler(MessageHandler message_handler)
    {
        _message_handler = std::move(message_handler);
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
        return _message_handler(std::move(message));
    }

private:
    CefRefPtr<CefBrowser> _browser;
    MessageHandler _message_handler; ///< Function to handle messages from the browser
};

} // namespace vortex::ui
