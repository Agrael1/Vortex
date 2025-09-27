#pragma once
#include <vortex/graphics.h>
#include <vortex/nodes/node_registry.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/ui/ui_app.h>
#include <vortex/model.h>
#include <vortex/util/lib/SPSC-Queue.h>
#include <vortex/ui/message_dispatch.h>
#include <vortex/util/ndi/ndi_library.h>
#include <vortex/util/main_args.h>
#include <vortex/sync/wall_clock.h>
#include <vortex/util/term/input.h>
#include <filesystem>
#include <fstream>

namespace vortex {
struct AppExitControl {

    AppExitControl() = default;
    AppExitControl(const AppExitControl&) = delete;

    static void Exit()
    {
        GetInstance().exit.store(true, std::memory_order::relaxed);
    }

    static AppExitControl& GetInstance()
    {
        static AppExitControl instance;
        return instance;
    }

    std::atomic<bool> exit = false;
};

class App
{
    using MessageHandler = void (App::*)(CefListValue&);
    using MessageHanlderDispatch = void (*)(App&, CefListValue&);

public:
    App(const MainArgs& args)
        : _gfx(true)
        , _exit(AppExitControl::GetInstance())
        , _ui_app(CreateUIApp(args.headless))
    {
        TerminalHandler::Instance().SetInputHandler([](std::string_view line, void* p) {
            return static_cast<App*>(p)->TerminalMessageHandler(line);
        },
                                                    this);

        wis::Result res = wis::success;
        vortex::UpdateNotifier::External external_observer{
            .observer = this,
            .callback = &App::OnNodeUpdateThunk
        };
        _ui_app.BindMessageHandler([this](CefRefPtr<CefProcessMessage> args) { return UIMessageHandler(std::move(args)); });

        constexpr std::pair<std::string_view, std::string_view> output_values2[]{
            std::pair{ "name", "Vortex Mega Output" },
            std::pair{ "window_size", "[2000,2000]" }
        };
        constexpr std::pair<std::string_view, std::string_view> output_values3[]{
            std::pair{ "name", "Vortex Mega Output 2" },
            std::pair{ "window_size", "[1000,2000]" },
            std::pair{ "framerate", "[30,1]" }
        };

        constexpr std::pair<std::string_view, std::string_view> stream_values[]{
            std::pair{ "stream_url", "rtp://127.0.0.1:1234" },
        };

        // Test setup of the model
        auto i1 = _model.CreateNode(_gfx, "StreamInput", external_observer, stream_values); // Create a default node for testing
        auto o1 = _model.CreateNode(_gfx, "WindowOutput", external_observer, output_values3); // Create a default output for testing
        auto o2 = _model.CreateNode(_gfx, "WindowOutput", external_observer, output_values2); // Create a default output for testing

        _model.SetNodeInfo(i1, "Image 1"); // Set some info for the node
        _model.SetNodeInfo(o1, "Output 0"); // Set some info for the output node
        _model.SetNodeInfo(o2, "Output 1"); // Set some info for the output node

        _model.ConnectNodes(i1, 0, o1, 0); // Connect the nodes in the model
        //_model.ConnectNodes(i1, 1, o1, 1); // Connect the audio outputs

        _model.ConnectNodes(i1, 0, o2, 0); // Connect the nodes in the model
    }

public:
    int Run()
    {
        while (!_exit.exit) {
            if (int code = _ui_app.ProcessEvents()) {
                return code; // Exit requested
            }

            // Process terminal input
            vortex::PollTerminalInput();

            // Process messages from the UI
            ProcessMessages();

            // Process the model and render the nodes
            _model.TraverseNodes(_gfx); // Traverse the nodes in the model
        }

        return 0;
    }

private:
    void ProcessMessages()
    {
        size_t size = _message_queue.size();
        CefRefPtr<CefProcessMessage> message;

        // Limit the number of messages processed in one frame to avoid blocking
        for (size_t i = 0; i < size; ++i) {
            if (!_message_queue.try_pop(message)) {
                break; // No more messages to process
            }
            std::invoke(_message_handlers_disp[message->GetName().c_str()], *this, *message->GetArgumentList()); // Call the appropriate handler
        }
    }
    bool UIMessageHandler(CefRefPtr<CefProcessMessage> message) noexcept
    {
        if (!message || !message->IsValid()) {
            return false; // Invalid message
        }
        auto message_name = message->GetName();
        if (auto it = _message_handlers_disp.find(message_name.c_str()); it != _message_handlers_disp.end()) {
            _message_queue.emplace(std::move(message)); // Add message to the queue
            return true; // Message handled
        }

        return false; // Message not handled
    }
    bool TerminalMessageHandler(std::string_view line) noexcept
    {
        if (line == "exit" || line == "quit") {
            AppExitControl::Exit();
            return true; // Message handled
        }

        // Simple command execution
        if (line.starts_with("exec ")) {
            auto command = line.substr(5);
            _ui_app.ExecuteJavaScript(command); //
            return true; // Message handled
        }

        // Execute file
        if (line.starts_with("execf ")) {
            auto filename = line.substr(6);
            std::filesystem::path path = filename;
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                std::ifstream file(path);
                if (file) {
                    std::string script((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    _ui_app.ExecuteJavaScript(script);
                    return true; // Message handled
                }
            }
        }

        return false; // Message not handled
    }

    // Message handlers for specific messages
    void GetNodeTypes()
    {
        const auto& node_types = vortex::graph::NodeFactory::GetNodesInfo();
        CefRefPtr<CefDictionaryValue> return_dictionary = CefDictionaryValue::Create();

        for (const auto& [name, info] : node_types) {
            return_dictionary->SetString({ name.data() }, serialize(info));
        }
        _ui_app.SendUIReturn(std::move(return_dictionary)); // Send the node types to the UI
    }
    auto GetNodeProperties(uintptr_t node_ptr) -> std::string
    {
        return _model.GetNodeProperties(node_ptr); // Send the properties to the UI
    }
    void SetNodeProperty(uintptr_t node_ptr, int index, std::string value)
    {
        _model.SetNodeProperty(node_ptr, uint32_t(index), value); // Set the property in the model
    }
    auto CreateNode(std::string value) -> uintptr_t
    {
        return _model.CreateNode(_gfx, value);
    }
    void RemoveNode(uintptr_t node_ptr)
    {
        _model.RemoveNode(node_ptr); // Delete the node with the specified ID
    }
    bool ConnectNodes(uintptr_t node_ptr_left, int32_t output_index, uintptr_t node_ptr_right, int32_t input_index)
    {
        return _model.ConnectNodes(node_ptr_left, output_index, node_ptr_right, input_index); // Connect the nodes in the model
    }
    void DisconnectNodes(uintptr_t node_ptr_left, int32_t output_index, uintptr_t node_ptr_right, int32_t input_index)
    {
        _model.DisconnectNodes(node_ptr_left, output_index, node_ptr_right, input_index); // Disconnect the nodes in the model
    }
    void SetNodeInfo(uintptr_t node_ptr, std::string info)
    {
        _model.SetNodeInfo(node_ptr, info); // Set the node info in the model
    }

private:
    // Thunk for node update observer
    static void OnNodeUpdateThunk(void* observer, uintptr_t node, uint32_t property_index, std::string_view value)
    {
        std::bit_cast<App*>(observer)->OnNodeUpdate(node, property_index, value);
    }
    static vortex::ui::UIApp CreateUIApp(bool headless)
    {
        if (headless) {
            return vortex::ui::UIApp();
        } else {
            return vortex::ui::UIApp("Vortex Application", 1920, 1080, false);
        }
    }
    void OnNodeUpdate(uintptr_t node, uint32_t property_index, std::string_view value)
    {
        // Handle node update logic here
        vortex::info("Node updated: {} (Property: {}, Value: {})", node, property_index, value);
        //_ui_app.SendUIMessage(u"node_update", std::bit_cast<double>(node), property_index, value);
    }

public:
    template<typename... Args>
    void HandleUIReturn(Args&&... args)
    {
        _ui_app.SendUIReturn(std::forward<Args>(args)...);
    }

private:
    vortex::NDILibrary _ndi;
    vortex::ui::SDLLibrary _sdl;

    vortex::Graphics _gfx;
    dro::SPSCQueue<CefRefPtr<CefProcessMessage>, 64> _message_queue; ///< Queue for messages from the UI

    // CEF client for UI
    vortex::ui::UIApp _ui_app;
    vortex::LazyToken _lazy_token; ///< Lazy token for removing lazy data before graphics shutdown
    vortex::graph::GraphModel _model; ///< Model containing nodes and outputs

    // Message handlers map - this should be a simple map lookup as these are
    // used in hot code, so it should be fast
    std::unordered_map<std::u16string_view, MessageHanlderDispatch> _message_handlers_disp{
        // Coroutines
        { u"GetNodeTypesAsync", ui::MessageDispatch<&App::GetNodeTypes>::Dispatch },
        { u"CreateNodeAsync", ui::MessageDispatch<&App::CreateNode>::Dispatch },
        { u"GetNodePropertiesAsync", ui::MessageDispatch<&App::GetNodeProperties>::Dispatch },

        // Immediate calls (fire and forget)
        { u"RemoveNode", ui::MessageDispatch<&App::RemoveNode>::Dispatch },
        { u"ConnectNodes", ui::MessageDispatch<&App::ConnectNodes>::Dispatch },
        { u"DisconnectNodes", ui::MessageDispatch<&App::DisconnectNodes>::Dispatch },
        { u"SetNodeInfo", ui::MessageDispatch<&App::SetNodeInfo>::Dispatch },
        { u"SetNodeProperty", ui::MessageDispatch<&App::SetNodeProperty>::Dispatch }
    };

private:
    const AppExitControl& _exit;
};
} // namespace vortex