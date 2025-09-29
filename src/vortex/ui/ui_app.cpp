#include <vortex/ui/ui_app.h>
#include <include/cef_browser.h>

#if defined(SDL_PLATFORM_LINUX)
#include <X11/Xlib.h>
#endif

void vortex::ui::UIApp::InitializeCEF()
{
    // Create CEF client
    _cef_client = new vortex::ui::Client();
    if (!_window) {
        // Headless mode, no window to create browser in
        CefBrowserSettings browser_settings;
        CefWindowInfo window_info;
        window_info.SetAsWindowless(nullptr); // Headless mode
        CefBrowserHost::CreateBrowser(window_info, _cef_client.get(),
                                      "about:blank", browser_settings, nullptr, nullptr);


        return;
    } 
    

    // Set up CEF window info to use SDL window as parent
    CefWindowInfo window_info;

    int width, height;
    SDL_GetWindowSizeInPixels(_window->GetWindow(), &width, &height);
    float scale = SDL_GetWindowDisplayScale(_window->GetWindow());
    width = static_cast<int>(width / scale);
    height = static_cast<int>(height / scale);

#ifdef SDL_PLATFORM_WIN32
    // Get the native Windows HWND from SDL window
    HWND parent_hwnd = (HWND)SDL_GetPointerProperty(
            SDL_GetWindowProperties(_window->GetWindow()),
            SDL_PROP_WINDOW_WIN32_HWND_POINTER,
            nullptr);

    if (parent_hwnd) {
        window_info.SetAsChild(parent_hwnd, CefRect(0, 0, width, height));
    }
#elif defined(SDL_PLATFORM_LINUX)
    // Linux X11 implementation
    ::Display* x_display = (::Display*)SDL_GetPointerProperty(
            SDL_GetWindowProperties(_window->GetWindow()),
            SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
            nullptr);

    ::Window x_window = (::Window)SDL_GetNumberProperty(
            SDL_GetWindowProperties(_window->GetWindow()),
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
            "http://vortex/ui/index.html", // Initial URL - change as needed
            browser_settings,
            nullptr,
            nullptr);
}

void vortex::ui::UIApp::ResizeCEFBrowser(int width, int height)
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
                SDL_GetWindowProperties(_window->GetWindow()),
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
