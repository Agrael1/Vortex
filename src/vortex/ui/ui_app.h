#pragma once
#include <vortex/ui/client.h>
#include <vortex/ui/sdl.h>
#include <vortex/ui/value.h>
#include <optional>

namespace vortex::ui {
class UIApp
{
    static bool EventWatchThunk(void* userdata, SDL_Event* event)
    {
        return static_cast<UIApp*>(userdata)->EventWatch(*event);
    }
    bool EventWatch(SDL_Event& event)
    {
        if (event.window.windowID == _window->GetID() && event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            int width = event.window.data1;
            int height = event.window.data2;
            ResizeCEFBrowser(width, height);
            return true; // Event handled
        }
        return false; // Event not handled
    }

public:
    // Headless mode constructor
    UIApp()
    {
        InitializeCEF();
    }
    UIApp(const char* window_title, int width, int height, bool fullscreen)
        : _window(std::in_place, window_title, width, height, fullscreen)
    {
        SDL_AddEventWatch(EventWatchThunk, this);
        // Initialize CEF
        InitializeCEF();
    }

public:
    int ProcessEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_QUIT:
                return 1;
            default:
                break;
            }
        }

        return 0;
    }
    void BindMessageHandler(Client::MessageHandler callback)
    {
        _cef_client->BindMessageHandler(std::move(callback));
    }
    CefRefPtr<Client> GetClient() const
    {
        return _cef_client;
    }

    template<typename... Args>
    void SendUIMessage(std::u16string_view message_name, Args&&... args)
    {
        if (!_cef_client) {
            vortex::error("UIApp::SendMessage: CEF client is not initialized.");
            return;
        }
        auto browser = _cef_client->GetBrowser();
        if (browser) {
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(string_traits::to_cef(message_name));
            auto msg_args = message->GetArgumentList();

            constexpr auto size = sizeof...(Args);
            msg_args->SetSize(size);

            // Add arguments to the message
            if constexpr (size > 0) {
                size_t i = 0;
                (value_traits<std::remove_reference_t<std::remove_all_extents_t<Args>>>::add_value(*msg_args, i++, std::forward<Args>(args)), ...);
            }

            // Send the message
            browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(message));
        }
    }
    template<typename... Args>
    void SendUIReturn(Args&&... args)
    {
        SendUIMessage(u"co_return", std::forward<Args>(args)...);
    }

    void ExecuteJavaScript(std::string_view script)
    {
        if (!_cef_client) {
            vortex::error("UIApp::ExecuteJavaScript: CEF client is not initialized.");
            return;
        }
        auto browser = _cef_client->GetBrowser();
        if (browser) {
            auto frame = browser->GetMainFrame();
            if (frame) {
                frame->ExecuteJavaScript(string_traits::to_cef(script), "", 0);
            }
        }
    }

private:
    void InitializeCEF();
    void ResizeCEFBrowser(int width, int height);

private:
    std::optional<SDLWindow> _window; ///< SDL window for the application
    CefRefPtr<Client> _cef_client; ///< CEF client for handling browser events
};
} // namespace vortex::ui