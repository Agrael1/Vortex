#pragma once
#include <vortex/graphics.h>
#include <vortex/node_registry.h>
#include <vortex/pipeline_storage.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/ui/ui_app.h>
#include <vortex/model.h>
#include <vortex/util/lib/SPSC-Queue.h>

namespace vortex {
struct AppExitControl {

    AppExitControl() = default;
    AppExitControl(const AppExitControl&) = delete;

    static void Exit()
    {
        GetInstance().exit = true;
    }

    static AppExitControl& GetInstance()
    {
        static AppExitControl instance;
        return instance;
    }

    bool exit = false;
};

class App
{
    using MessageHandler = void (App::*)(CefListValue&);

public:
    App()
        : _gfx(true)
        , _exit(AppExitControl::GetInstance())
        , _ui_app("Vortex Application", 1920, 1080, false)
        , _descriptor_buffer(_gfx)
    {
        wis::Result res = wis::success;
        for (size_t i = 0; i < max_frames_in_flight; ++i) {
            _command_list[i] = _gfx.GetDevice().CreateCommandList(res, wis::QueueType::Graphics);
        }
        fence = _gfx.GetDevice().CreateFence(res);

        _ui_app.BindMessageHandler([this](CefRefPtr<CefProcessMessage> args) { return UIMessageHandler(std::move(args)); });
        auto i1 = _model.CreateNode(_gfx, "ImageInput"); // Create a default node for testing
        _model.SetNodeInfo(i1, "Image 0"); // Set some info for the node
        auto o1 = _model.CreateNode(_gfx, "WindowOutput"); // Create a default output for testing
        auto o2 = _model.CreateNode(_gfx, "NDIOutput"); // Create a default output for testing
        _model.SetNodeInfo(o1, "Output 0"); // Set some info for the output node
        _model.SetNodeProperty(o1, 0, "Vortex Mega Output"); // Set a property for the output node
        _model.SetNodeProperty(o1, 1, "[1080,1920]");
        _model.SetNodeInfo(o2, "NDI Output 0"); // Set some info for the NDI output node
        _model.ConnectNodes(i1, 0, o1, 0); // Connect the nodes in the model
        _model.ConnectNodes(i1, 0, o2, 0); // Connect the input node to the NDI output node

        _model.SetNodeProperty(i1, 0, "ui/HDR.jpg", true); // Set a property for the input node
    }

public:
    int Run()
    {
        while (!_exit.exit) {
            if (int code = _ui_app.ProcessEvents()) {
                return code; // Exit requested
            }
            ProcessMessages(); // Process messages from the UI

            // Process the model and render the nodes
            vortex::RenderProbe probe{
                _descriptor_buffer,
                _pipeline_storage,
                &_command_list[frame_index],
                {},
                nullptr,

                1,
                frame_index
            };
            _model.TraverseNodes(_gfx, probe); // Traverse the nodes in the model

            _gfx.GetMainQueue().SignalQueue(fence, fence_value);

            frame_index = (frame_index + 1) % max_frames_in_flight;
            fence.Wait(fence_values[frame_index]);

            fence_values[frame_index] = ++fence_value;
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
            std::invoke(_message_handlers[message->GetName().c_str()], this, *message->GetArgumentList()); // Call the appropriate handler
        }
    }
    bool UIMessageHandler(CefRefPtr<CefProcessMessage> message) noexcept
    {
        if (!message || !message->IsValid()) {
            return false; // Invalid message
        }
        auto message_name = message->GetName();
        if (auto it = _message_handlers.find(message_name.c_str()); it != _message_handlers.end()) {
            _message_queue.emplace(std::move(message)); // Add message to the queue
            return true; // Message handled
        }

        return false; // Message not handled
    }

    // Message handlers for specific messages
    void CreateNode(CefListValue& args)
    {
        if (args.GetSize() > 0 && args.GetType(0) == VTYPE_STRING) {
            std::string func_name = args.GetString(0).ToString();
            // Call the corresponding function in the model or handle it
            uintptr_t node_ptr = _model.CreateNode(_gfx, func_name);

            // Send a message back to the UI to update the node list
            _ui_app.SendUIReturn(std::bit_cast<double>(node_ptr));
        }
    }
    void RemoveNode(CefListValue& args)
    {
        if (args.GetSize() > 0 && args.GetType(0) == VTYPE_DOUBLE) {
            uintptr_t node_id = std::bit_cast<uintptr_t>(args.GetDouble(0));
            _model.RemoveNode(node_id); // Delete the node with the specified ID
        }
    }
    void ConnectNodes(CefListValue& args)
    {
        if (args.GetSize() < 4 || args.GetType(0) != VTYPE_DOUBLE || args.GetType(1) != VTYPE_INT || args.GetType(2) != VTYPE_DOUBLE || args.GetType(3) != VTYPE_INT) {
            vortex::error("ConnectNodes: Invalid arguments provided.");
            _ui_app.SendUIReturn(false); // Send an error message back to the UI
            return; // Invalid arguments, cannot connect nodes
        }

        // Extract the node pointers and indices from the arguments
        uintptr_t node_ptr_left = std::bit_cast<uintptr_t>(args.GetDouble(0));
        int32_t output_index = args.GetInt(1);
        uintptr_t node_ptr_right = std::bit_cast<uintptr_t>(args.GetDouble(2));
        int32_t input_index = args.GetInt(3);
        _model.ConnectNodes(node_ptr_left, output_index, node_ptr_right, input_index); // Connect the nodes in the model

        // Create a message to send back to the UI
        _ui_app.SendUIReturn(true); // Indicate that the connection was successful
    }
    void DisconnectNodes(CefListValue& args)
    {
        if (args.GetSize() < 4 || args.GetType(0) != VTYPE_DOUBLE || args.GetType(1) != VTYPE_INT || args.GetType(2) != VTYPE_DOUBLE || args.GetType(3) != VTYPE_INT) {
            vortex::error("DisconnectNodes: Invalid arguments provided.");
            _ui_app.SendUIReturn(false); // Send an error message back to the UI
            return; // Invalid arguments, cannot disconnect nodes
        }
        // Extract the node pointers and indices from the arguments
        uintptr_t node_ptr_left = std::bit_cast<uintptr_t>(args.GetDouble(0));
        int32_t output_index = args.GetInt(1);
        uintptr_t node_ptr_right = std::bit_cast<uintptr_t>(args.GetDouble(2));
        int32_t input_index = args.GetInt(3);
        _model.DisconnectNodes(node_ptr_left, output_index, node_ptr_right, input_index); // Disconnect the nodes in the model
        // Create a message to send back to the UI
        _ui_app.SendUIReturn(true); // Indicate that the disconnection was successful
    }
    void SetNodeInfo(CefListValue& args)
    {
        if (args.GetSize() < 2 || args.GetType(0) != VTYPE_DOUBLE || args.GetType(1) != VTYPE_STRING) {
            vortex::error("SetNodeInfo: Invalid arguments provided.");
            return; // Invalid arguments, cannot set node info
        }
        uintptr_t node_ptr = std::bit_cast<uintptr_t>(args.GetDouble(0));
        _model.SetNodeInfo(node_ptr, args.GetString(1).ToString()); // Set the node info in the model
    }

    void GreetAsync(CefListValue& args2)
    {
        _ui_app.SendUIReturn(counter++);
    }

private:
    vortex::NDILibrary _ndi;
    vortex::ui::SDLLibrary _sdl;

    vortex::Graphics _gfx;
    vortex::PipelineStorage _pipeline_storage;
    vortex::DescriptorBuffer _descriptor_buffer;

    dro::SPSCQueue<CefRefPtr<CefProcessMessage>, 64> _message_queue; ///< Queue for messages from the UI

    wis::CommandList _command_list[max_frames_in_flight]; ///< Command list for recording commands
    uint32_t _current_frame = 0; ///< Current frame index for command list

    wis::Fence fence;
    uint64_t fence_value = 1;
    uint64_t frame_index = 0;
    std::array<uint64_t, max_frames_in_flight> fence_values{ 1, 0 };

    // CEF client for UI
    vortex::ui::UIApp _ui_app;
    vortex::graph::GraphModel _model; ///< Model containing nodes and outputs

    int32_t counter = 32; ///< Counter for async calls

    // used in hot code, so it should be fast
    std::unordered_map<std::u16string_view, MessageHandler> _message_handlers{
        { u"CreateNode", &App::CreateNode },
        { u"RemoveNode", &App::RemoveNode },
        { u"ConnectNodes", &App::ConnectNodes },
        { u"DisconnectNodes", &App::DisconnectNodes },
        { u"SetNodeInfo", &App::SetNodeInfo },
        { u"GreetAsync", &App::GreetAsync }
    };

private:
    const AppExitControl& _exit;
};
} // namespace vortex