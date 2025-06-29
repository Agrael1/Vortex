#pragma once
#include <vortex/ui/implements.h>
#include <include/cef_client.h>

namespace vortex::ui {
class Client : public CefImplements<Client, CefClient, CefLifeSpanHandler>
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
    CefBrowser* GetBrowser() noexcept
    {
        return _browser.get();
    }

private:
    CefRefPtr<CefBrowser> _browser;
};

} // namespace vortex::ui
