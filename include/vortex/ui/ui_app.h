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
            vortex::info("SDL Event: {}", event.type);

            switch (event.type) {
            case SDL_EVENT_QUIT:
                return 1;
            default:
                break;
            }
        }

        return 0;
    }

private:
    void InitializeCEF()
    {
        // Create CEF client
        _cef_client = new vortex::ui::Client();

        // Set up CEF window info to use SDL window as parent
        CefWindowInfo window_info;

        int width, height;
        SDL_GetWindowSizeInPixels(_window.GetWindow(), &width, &height);
        float scale = SDL_GetWindowDisplayScale(_window.GetWindow());
        width = static_cast<int>(width / scale);
        height = static_cast<int>(height / scale);

#ifdef SDL_PLATFORM_WIN32
        // Get the native Windows HWND from SDL window
        HWND parent_hwnd = (HWND)SDL_GetPointerProperty(
                SDL_GetWindowProperties(_window.GetWindow()),
                SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                NULL);

        if (parent_hwnd) {
            window_info.SetAsChild(parent_hwnd, CefRect(0, 0, width, height));
        }
#elif defined(SDL_PLATFORM_LINUX)
        // Linux X11 implementation
        Display* x_display = (Display*)SDL_GetPointerProperty(
                SDL_GetWindowProperties(_window.GetWindow()),
                SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                NULL);

        ::Window x_window = (::Window)SDL_GetNumberProperty(
                SDL_GetWindowProperties(_window.GetWindow()),
                SDL_PROP_WINDOW_X11_WINDOW_NUMBER,
                0);

        if (x_display && x_window) {
            window_info.SetAsChild(x_window, CefRect(0, 0, width, height));
        }
#endif

        // Browser settings
        CefBrowserSettings browser_settings;

        // Create the browser
        CefBrowserHost::CreateBrowser(
                window_info,
                _cef_client,
                "https://www.youtube.com/watch?v=R7U0fcTIpY0", // Initial URL - change as needed
                browser_settings,
                nullptr,
                nullptr);
    }
    void ResizeCEFBrowser(int width, int height)
    {
        if (auto* browser = _cef_client->GetBrowser()) {
            auto host = browser->GetHost();
            host->NotifyMoveOrResizeStarted();
#ifdef SDL_PLATFORM_WIN32
            // Get the CEF browser's window handle
            HWND cef_hwnd = host->GetWindowHandle();
            if (cef_hwnd) {
                // Resize the CEF browser window to match the SDL window
                SetWindowPos(cef_hwnd, NULL, 0, 0, width, height,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
#elif defined(SDL_PLATFORM_LINUX)
            // For Linux, you'd use X11 functions to resize the window
            Display* display = (Display*)SDL_GetPointerProperty(
                    SDL_GetWindowProperties(_window.GetWindow()),
                    SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                    NULL);

            if (display) {
                ::Window cef_window = host->GetWindowHandle();
                if (cef_window) {
                    XResizeWindow(display, cef_window, width, height);
                    XFlush(display);
                }
            }
#endif

            // Notify CEF of the resize
            host->WasResized();
        }
    }

private:
    SDLWindow _window; ///< SDL window for the application
    CefRefPtr<Client> _cef_client; ///< CEF client for handling browser events
};
} // namespace vortex::ui