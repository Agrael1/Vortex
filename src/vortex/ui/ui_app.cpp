#include <vortex/ui/ui_app.h>
#include <include/cef_browser.h>
#include <include/cef_dialog_handler.h>

namespace {
class FileDialogCallback : public CefRunFileDialogCallback
{
public:
    using ResultHandler = std::function<void(std::vector<std::filesystem::path>)>;

    explicit FileDialogCallback(ResultHandler handler)
        : _handler(std::move(handler))
    {
    }

    void OnFileDialogDismissed(const std::vector<CefString>& file_paths) override
    {
        std::vector<std::filesystem::path> paths;
        paths.reserve(file_paths.size());
        for (const auto& file : file_paths) {
            paths.emplace_back(file.ToString());
        }

        if (_handler) {
            _handler(std::move(paths));
        }
    }

private:
    ResultHandler _handler;
    IMPLEMENT_REFCOUNTING(FileDialogCallback);
};
} // namespace

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

void vortex::ui::UIApp::ShowOpenFileDialog(const std::vector<std::string>& filters,
                                           std::function<void(std::vector<std::filesystem::path>)> callback,
                                           std::string title)
{
    if (!_cef_client) {
        if (callback) {
            callback({});
        }
        return;
    }

    if (auto* browser = _cef_client->GetBrowser()) {
        auto host = browser->GetHost();
        if (!host) {
            if (callback) {
                callback({});
            }
            return;
        }

        std::vector<CefString> cef_filters;
        cef_filters.reserve(filters.size());
        for (const auto& filter : filters) {
            cef_filters.emplace_back(filter);
        }

        // cef_file_dialog_mode_t does not expose named constants via the C++ alias.
        constexpr auto kOpenDialogMode = static_cast<CefBrowserHost::FileDialogMode>(0); // FILE_DIALOG_OPEN

        const auto dialog_title = title.empty() ? CefString("Select file") : CefString(title);
        host->RunFileDialog(kOpenDialogMode,
            dialog_title,
                CefString(),
                cef_filters,
                new FileDialogCallback(std::move(callback)));
    } else if (callback) {
        callback({});
    }
}

void vortex::ui::UIApp::ShowSelectFolderDialog(std::function<void(std::vector<std::filesystem::path>)> callback)
{
    if (!_cef_client) {
        if (callback) {
            callback({});
        }
        return;
    }

    if (auto* browser = _cef_client->GetBrowser()) {
        auto host = browser->GetHost();
        if (!host) {
            if (callback) {
                callback({});
            }
            return;
        }

        // Value 2 corresponds to FILE_DIALOG_OPEN_FOLDER in cef_file_dialog_mode_t.
        constexpr auto kFolderDialogMode = static_cast<CefBrowserHost::FileDialogMode>(2);

        host->RunFileDialog(kFolderDialogMode,
            CefString("Select project folder"),
                CefString(),
                {},
                new FileDialogCallback(std::move(callback)));
    } else if (callback) {
        callback({});
    }
}
