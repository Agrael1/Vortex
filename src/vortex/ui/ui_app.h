#pragma once
#include <vortex/ui/client.h>
#include <vortex/ui/sdl.h>

namespace vortex::ui {
class UIApp
{
    static bool EventWatchThunk(void* userdata, SDL_Event* event)
    {
        return static_cast<UIApp*>(userdata)->EventWatch(*event);
    }
    bool EventWatch(SDL_Event& event)
    {
        if (event.window.windowID == _window.GetID() && event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            int width = event.window.data1;
            int height = event.window.data2;
            ResizeCEFBrowser(width, height);
            return true; // Event handled
        }
        return false; // Event not handled
    }

public:
    UIApp(const char* window_title, int width, int height, bool fullscreen)
        : _window(window_title, width, height, fullscreen)
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
            case SDL_EVENT_QUIT:
                return 1;
            default:
                break;
            }
        }

        return 0;
    }
    void BindFunction(const std::string& name, std::function<void(CefListValue&)> callback)
    {
        _cef_client->BindFunction(name, callback);
    }
    CefRefPtr<Client> GetClient() const
    {
        return _cef_client;
    }

private:
    void InitializeCEF();
    void ResizeCEFBrowser(int width, int height);

private:
    SDLWindow _window; ///< SDL window for the application
    CefRefPtr<Client> _cef_client; ///< CEF client for handling browser events
};
} // namespace vortex::ui