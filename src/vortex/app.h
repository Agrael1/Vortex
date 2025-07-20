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
        auto o1 = _model.CreateNode(_gfx, "WindowOutput"); // Create a default output for testing
        _model.ConnectNodes(i1, o1); // Connect the nodes in the model
    }

public:
    int Run()
    {
        while (!_exit.exit) {
            if (int code = _ui_app.ProcessEvents()) {
                return code; // Exit requested
            }
            ProcessMessages(); // Process messages from the UI

            //// Process the model and render the nodes
            // vortex::RenderProbe probe{
            //     _gfx,
            //     _descriptor_buffer,
            //     _pipeline_storage,
            //     _command_list[frame_index],
            //     {},
            //     nullptr,

            //    1,
            //    frame_index
            //};
            //_model.TraverseNodes(probe); // Traverse the nodes in the model

            //_gfx.GetMainQueue().SignalQueue(fence, fence_value);

            // frame_index = (frame_index + 1) % max_frames_in_flight;
            // fence.Wait(fence_values[frame_index]);

            // fence_values[frame_index] = ++fence_value;
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
    vortex::GraphModel _model; ///< Model containing nodes and outputs

    int32_t counter = 32; ///< Counter for async calls

    // used in hot code, so it should be fast
    std::unordered_map<std::u16string_view, MessageHandler> _message_handlers{
        { u"CreateNode", &App::CreateNode },
        { u"RemoveNode", &App::RemoveNode },
        { u"GreetAsync", &App::GreetAsync }
    };

private:
    const AppExitControl& _exit;
};
} // namespace vortex