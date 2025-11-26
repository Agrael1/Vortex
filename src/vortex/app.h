#pragma once
#include <vortex/graphics.h>
#include <vortex/nodes/node_registry.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/ui/ui_app.h>
#include <vortex/ui/message_routing.h>
#include <vortex/model.h>
#include <vortex/util/lib/SPSC-Queue.h>
#include <vortex/ui/message_dispatch.h>
#include <vortex/util/ndi/ndi_library.h>
#include <vortex/util/main_args.h>
#include <vortex/sync/wall_clock.h>
#include <vortex/util/term/input.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <format>
#include <sstream>
#include <iomanip>
#include <functional>
#include <system_error>
#include <optional>
#include <chrono>
#include <cctype>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <limits>
#include <cmath>
#include <exception>
#include <bit>
#include <include/cef_parser.h>
#include <nlohmann/json.hpp>
#include <vortex/project_commands.h>

namespace vortex {
using json = nlohmann::json;
using Clock = std::chrono::steady_clock;
struct AppExitControl {

    AppExitControl() = default;
    AppExitControl(const AppExitControl&) = delete;

    static void Exit() { GetInstance().exit.store(true, std::memory_order::relaxed); }

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
    enum class TransportState {
        Ready,
        Playing,
        Paused,
        Error,
    };

public:
    App(const MainArgs& args)
        : _gfx(true)
        , _exit(AppExitControl::GetInstance())
        , _ui_app(CreateUIApp(args.headless))
    {
        TerminalHandler::Instance().SetInputHandler(
                [](std::string_view line, void* p) {
                    return static_cast<App*>(p)->TerminalMessageHandler(line);
                },
                this);

        wis::Result res = wis::success;
        vortex::UpdateNotifier::External external_observer{ .observer = this,
                                                            .callback = &App::OnNodeUpdateThunk };
        _ui_app.BindMessageHandler([this](CefRefPtr<CefProcessMessage> args) {
            return UIMessageHandler(std::move(args));
        });

        _model.SetEdgeEventCallbacks(
                [this](uintptr_t sourcePtr,
                       int32_t sourceSlot,
                       uintptr_t targetPtr,
                       int32_t targetSlot) {
                    EmitEdgeConnected(sourcePtr, sourceSlot, targetPtr, targetSlot);
                },
                [this](uintptr_t sourcePtr,
                       int32_t sourceSlot,
                       uintptr_t targetPtr,
                       int32_t targetSlot) {
                    EmitEdgeDisconnected(sourcePtr, sourceSlot, targetPtr, targetSlot);
                });

        // constexpr std::pair<std::string_view, std::string_view> output_values2[]{
        //     std::pair{        "name", "Vortex Mega Output" },
        //     std::pair{ "window_size",        "[2000,2000]" }
        // };
        // constexpr std::pair<std::string_view, std::string_view> output_values3[]{
        //     std::pair{        "name", "Vortex Mega Output 2" },
        //     std::pair{ "window_size",          "[1000,2000]" },
        //     std::pair{   "framerate",               "[30,1]" }
        // };

        // constexpr std::pair<std::string_view, std::string_view> stream_values[]{
        //     std::pair{ "stream_url", "rtp://192.168.100.5:6970" },
        // };
        // constexpr std::pair<std::string_view, std::string_view> image_values[]{
        //     std::pair{ "image_path", "ui/HDR.jpg" },
        // };
        // constexpr std::pair<std::string_view, std::string_view> image_values2[]{
        //     std::pair{ "image_path", "ui/Watermark.png" },
        // };
        // constexpr std::pair<std::string_view, std::string_view> image_values3[]{
        //     std::pair{ "blend_mode", "0" },
        // };

        //// Test setup of the model
        // auto i1 = _model.CreateNode(_gfx,
        //                             "StreamInput",
        //                             external_observer,
        //                             stream_values); // Create a default node for testing
        //// auto o1 = _model.CreateNode(_gfx,
        ////                             "WindowOutput",
        ////                             external_observer,
        ////                             output_values3); // Create a default output for testing
        // auto o2 = _model.CreateNode(_gfx,
        //                             "NDIOutput",
        //                             external_observer,
        //                             output_values2); // Create a default output for testing
        // auto b1 = _model.CreateNode(_gfx, "Blend", external_observer, image_values3);
        // auto i2 = _model.CreateNode(_gfx, "ImageInput", external_observer, image_values);
        //// auto i3 = _model.CreateNode(_gfx, "ImageInput", external_observer, image_values2);
        // auto s1 = _model.CreateNode(_gfx, "Select", external_observer);

        //_model.SetNodeInfo(i1, "Stream 1"); // Set some info for the node
        //_model.SetNodeInfo(i2, "Image 1"); // Set some info for the node
        ////_model.SetNodeInfo(i3, "Image 2"); // Set some info for the node
        ////_model.SetNodeInfo(o1, "Output 0"); // Set some info for the output node
        //_model.SetNodeInfo(o2, "Output 1"); // Set some info for the output node

        ////_model.ConnectNodes(i1, 0, o1, 0); // Connect the nodes in the model
        ////_model.ConnectNodes(i1, 1, o2, 1); // Connect the audio outputs

        //_model.ConnectNodes(i1, 0, s1, 0); // Connect the nodes in the model
        //_model.ConnectNodes(i2, 0, s1, 1); // Connect the nodes in the model

        //// Blend 2 images
        ////_model.ConnectNodes(i1, 0, b1, 0); // Connect the nodes in the model
        ////_model.ConnectNodes(i3, 0, b1, 1); // Connect the nodes in the model

        //_model.ConnectNodes(s1, 0, o2, 0); // Connect the nodes in the model

        //// Animate
        // auto animation = _model.CreateAnimation(s1);
        // auto track = _model.AddPropertyTrack(animation, "input_index", {});
        //_model.AddKeyframe(track, R"({ "time_from_start" : 180000, "value" : 1 })");
    }

public:
    int Run()
    {
        //_model.Play(); // Start the model processing
        while (!_exit.exit) {
            if (int code = _ui_app.ProcessEvents()) {
                return code; // Exit requested
            }

            // Process terminal input
            vortex::PollTerminalInput();

            // Process messages from the UI
            ProcessMessages();

            // Process the model and render the nodes
            try {
                _model.TraverseNodes(_gfx); // Traverse the nodes in the model
            } catch (const std::exception& ex) {
                vortex::error("Graph traversal failed: {}", ex.what());
                HandleTransportFailure(std::string(ex.what()));
            } catch (...) {
                vortex::error("Graph traversal failed with unknown error");
                HandleTransportFailure("Graph traversal failed with unknown error");
            }

            MaybeEmitTransportMetrics();

            TickProjectPersistence();
        }

        return 0;
    }
    void MaybeEmitTransportMetrics()
    {
        if (!_model.IsPlaying()) {
            return;
        }

        constexpr auto kEmitInterval = std::chrono::milliseconds(750);
        constexpr double kIdleFpsThreshold = 0.25; // Treat scheduler as idle once no frames land
        const auto now = Clock::now();
        if (_next_transport_metrics_emit != Clock::time_point{} && now < _next_transport_metrics_emit) {
            return;
        }

        _next_transport_metrics_emit = now + kEmitInterval;

        const auto stats = _model.SampleTransportStats();
        const double resolved_fps = std::isfinite(stats.fps) && stats.fps > 0.0 ? stats.fps : 0.0;
        const bool has_outputs = !_model.GetOutputs().empty();
        const bool scheduler_idle = resolved_fps <= kIdleFpsThreshold;
        const bool should_report_idle = !has_outputs || scheduler_idle;

        auto append_reason = [](std::string& base, std::string_view extra) {
            if (extra.empty()) {
                return;
            }
            if (!base.empty()) {
                base.append("; ");
            }
            base.append(extra);
        };

        std::string reason;
        if (!has_outputs) {
            append_reason(reason, "No active outputs configured");
        } else if (scheduler_idle) {
            append_reason(reason, "Transport idle (no rendered frames in the last window)");
        }

        if (stats.dropped_frames > 0) {
            if (!stats.last_drop_hint.empty()) {
                append_reason(reason,
                              std::format("{} dropped frame{} in the last window (last: {})",
                                          stats.dropped_frames,
                                          stats.dropped_frames == 1 ? "" : "s",
                                          stats.last_drop_hint));
            } else {
                append_reason(reason,
                              std::format("{} dropped frame{} in the last window",
                                          stats.dropped_frames,
                                          stats.dropped_frames == 1 ? "" : "s"));
            }
        }

        const double fps_value = should_report_idle && resolved_fps == 0.0 ? GetActiveProjectFPS() : resolved_fps;
        std::optional<uintptr_t> drop_ptr;
        if (stats.last_drop_ptr != 0) {
            drop_ptr = stats.last_drop_ptr;
        }

        EmitTransportStateSnapshot(_transport_state,
                       fps_value,
                       stats.dropped_frames,
                       reason,
                       stats.last_drop_hint,
                       drop_ptr);
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
            auto meta_it = _pending_message_meta.find(message.get());
            if (meta_it == _pending_message_meta.end()) {
                vortex::warn("No metadata found for UI message {}", message->GetName().ToString());
                continue;
            }

            auto metadata = meta_it->second;
            _pending_message_meta.erase(meta_it);

            auto previous_request_id = _active_request_id;
            _active_request_id = metadata.request_id;

            std::invoke(metadata.dispatch,
                        *this,
                        *message->GetArgumentList()); // Call the appropriate handler

            _active_request_id = previous_request_id;
        }
    }
    bool UIMessageHandler(CefRefPtr<CefProcessMessage> message) noexcept
    {
        if (!message || !message->IsValid()) {
            return false; // Invalid message
        }
        auto routing = ui::ParseRoutedMessage(message->GetName());
        std::u16string_view handler_name(routing.base_name);
        if (auto it = _message_handlers_disp.find(handler_name);
            it != _message_handlers_disp.end()) {
            auto ptr = message.get();
            _pending_message_meta.emplace(ptr, MessageMetadata{ it->second, routing.request_id });
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
            static bool init_done = false;
            if (!init_done) {
                _ui_app.ExecuteJavaScript(R"(
                    const Vortex = new Proxy({}, {
    get(target, methodName) {
        const methodStr = String(methodName);

        // If method ends with 'Async', use vortexCallAsync
        if (methodStr.endsWith('Async')) {
            return function (...args) {
                return vortexCallAsync(methodName, ...args);
            };
        }

        // Return a function that calls vortexCall
        return function (...args) {
            return vortexCall(methodName, ...args);
        };
    }
});
                )");

                init_done = true;
            }

            auto filename = line.substr(6);
            std::filesystem::path path = filename;
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                std::ifstream file(path);
                if (file) {
                    std::string script((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    _ui_app.ExecuteJavaScript(script);
                    return true; // Message handled
                }
            }
        }

        return false; // Message not handled
    }

    static std::string BuildEdgeId(uintptr_t source_ptr,
                                   int32_t source_slot,
                                   uintptr_t target_ptr,
                                   int32_t target_slot)
    {
        return std::format("{}:{}->{}:{}",
                           source_ptr,
                           source_slot,
                           target_ptr,
                           target_slot);
    }

    void EmitEdgeConnected(uintptr_t source_ptr,
                           int32_t source_slot,
                           uintptr_t target_ptr,
                           int32_t target_slot)
    {
        _ui_app.SendUIMessage(u"edge_connected",
                              std::bit_cast<double>(source_ptr),
                              std::bit_cast<double>(target_ptr),
                              BuildEdgeId(source_ptr, source_slot, target_ptr, target_slot),
                              source_slot,
                              target_slot);
    }

    void EmitEdgeDisconnected(uintptr_t source_ptr,
                              int32_t source_slot,
                              uintptr_t target_ptr,
                              int32_t target_slot)
    {
        _ui_app.SendUIMessage(u"edge_disconnected",
                              std::bit_cast<double>(source_ptr),
                              std::bit_cast<double>(target_ptr),
                              BuildEdgeId(source_ptr, source_slot, target_ptr, target_slot),
                              source_slot,
                              target_slot);
    }

    void EmitGraphNodeCreatedEvent(const json& node,
                                   std::string_view client_id,
                                   uintptr_t node_ptr)
    {
        if (!node.is_object()) {
            return;
        }

        const std::string node_id = node.contains("id") && node["id"].is_string()
                ? TrimCopy(node["id"].get<std::string>())
                : std::string();
        const std::string node_type = node.contains("type") && node["type"].is_string()
                ? TrimCopy(node["type"].get<std::string>())
                : std::string();
        const std::string node_label = node.contains("label") && node["label"].is_string()
                ? TrimCopy(node["label"].get<std::string>())
                : std::string();

        double x = 0.0;
        double y = 0.0;
        if (node.contains("position") && node["position"].is_object()) {
            const auto& pos = node["position"];
            if (pos.contains("x") && pos["x"].is_number()) {
                x = pos["x"].get<double>();
            }
            if (pos.contains("y") && pos["y"].is_number()) {
                y = pos["y"].get<double>();
            }
        }

        std::string props_json;
        if (node.contains("props") && node["props"].is_object()) {
            props_json = node["props"].dump();
        }

        _ui_app.SendUIMessage(u"graph_node_created",
                      static_cast<double>(node_ptr),
                              node_id,
                              node_type,
                              node_label,
                              x,
                              y,
                              std::string(client_id),
                              props_json);
    }

    void EmitTransportStateSnapshot(TransportState state,
                                    std::optional<double> fps_override = std::nullopt,
                                    std::optional<uint32_t> dropped_frames = std::nullopt,
                                    std::string_view reason = {},
                                    std::string_view drop_hint = {},
                                    std::optional<uintptr_t> drop_target_ptr = std::nullopt)
    {
        auto resolve_fps = [&]() noexcept {
            if (fps_override.has_value() && std::isfinite(*fps_override)) {
                return *fps_override;
            }
            const double project_fps = GetActiveProjectFPS();
            return std::isfinite(project_fps) ? project_fps : 0.0;
        };

        const double fps_value = resolve_fps();
        const double dropped_value = dropped_frames.has_value()
                ? static_cast<double>(*dropped_frames)
                : 0.0;
        std::string reason_text(reason);
        std::string drop_text(drop_hint);
        const std::string timestamp = MakeIsoTimestamp();
        const auto drop_target_value = [&]() -> std::optional<double> {
            if (!drop_target_ptr.has_value()) {
                return std::nullopt;
            }
            const double candidate = std::bit_cast<double>(*drop_target_ptr);
            if (!std::isfinite(candidate)) {
                return std::nullopt;
            }
            return candidate;
        }();

        if (drop_target_value.has_value()) {
            _ui_app.SendUIMessage(u"transport_state",
                                  std::string(TransportStateToString(state)),
                                  fps_value,
                                  reason_text,
                                  timestamp,
                                  dropped_value,
                                  drop_text,
                                  *drop_target_value);
            return;
        }

        _ui_app.SendUIMessage(u"transport_state",
                              std::string(TransportStateToString(state)),
                              fps_value,
                              reason_text,
                              timestamp,
                              dropped_value,
                              drop_text);
    }

    void SetTransportState(TransportState state, std::string_view reason = {})
    {
        _transport_state = state;
        EmitTransportStateSnapshot(state, std::nullopt, std::nullopt, reason);
    }

    static constexpr std::string_view TransportStateToString(TransportState state) noexcept
    {
        switch (state) {
        case TransportState::Playing:
            return "playing";
        case TransportState::Paused:
            return "paused";
        case TransportState::Error:
            return "error";
        case TransportState::Ready:
        default:
            return "ready";
        }
    }

    void HandleTransportFailure(std::string reason)
    {
        if (_transport_state == TransportState::Error) {
            return;
        }
        vortex::error("Transport failure: {}", reason);
        _model.Stop();
        SetTransportState(TransportState::Error, reason);
    }

    double GetActiveProjectFPS() const noexcept
    {
        constexpr double kDefaultFPS = 60.0;
        if (!_active_project) {
            return kDefaultFPS;
        }

        try {
            const auto& snapshot = _active_project->snapshot;
            if (snapshot.contains("settings") && snapshot["settings"].is_object()) {
                const auto& settings = snapshot["settings"];
                if (settings.contains("fps") && settings["fps"].is_number_float()) {
                    return settings["fps"].get<double>();
                }
                if (settings.contains("fps") && settings["fps"].is_number_integer()) {
                    return static_cast<double>(settings["fps"].get<int64_t>());
                }

                if (settings.contains("framerate") && settings["framerate"].is_array() &&
                    settings["framerate"].size() >= 2 &&
                    settings["framerate"][0].is_number() &&
                    settings["framerate"][1].is_number()) {
                    const double num = settings["framerate"][0].get<double>();
                    const double denom = settings["framerate"][1].get<double>();
                    if (denom != 0.0) {
                        return num / denom;
                    }
                }
            }
        } catch (const std::exception& ex) {
            vortex::warn("Failed to resolve project FPS: {}", ex.what());
        }

        return kDefaultFPS;
    }

    // Message handlers for specific messages
    void GetNodeTypes()
    {
        const auto& node_types = vortex::graph::NodeFactory::GetNodesInfo();
        CefRefPtr<CefDictionaryValue> ret = CefDictionaryValue::Create();

        for (const auto& [name, info] : node_types) {
            // name — std::string or std::string_view
            std::string key(name);
            std::string value = serialize(info); // JSON/string
            ret->SetString(CefString(key), CefString(value));
            // alternative: ret->SetString(CefString(key), value);
        }

        HandleUIReturn(std::move(ret));
    }
    auto GetNodeProperties(uintptr_t node_ptr) -> std::string
    {
        return _model.GetNodeProperties(node_ptr); // Send the properties to the UI
    }
    void SetNodeProperty(uintptr_t node_ptr, int index, std::string value)
    {
        _model.SetNodeProperty(node_ptr, uint32_t(index), value); // Set the property in the model
    }
    void SetNodePropertyByName(uintptr_t node_ptr, std::string name, std::string value)
    {
        _model.SetNodePropertyByName(node_ptr, name, value); // Set the property in the model
    }
    auto CreateNode(std::string value) -> uintptr_t { return _model.CreateNode(_gfx, value); }
    void RemoveNode(uintptr_t node_ptr)
    {
        _model.RemoveNode(node_ptr); // Delete the node with the specified ID
    }
    bool ConnectNodes(uintptr_t node_ptr_left,
                      int32_t output_index,
                      uintptr_t node_ptr_right,
                      int32_t input_index)
    {
        return _model.ConnectNodes(node_ptr_left,
                                   output_index,
                                   node_ptr_right,
                                   input_index); // Connect the nodes in the model
    }
    bool DisconnectNodes(uintptr_t node_ptr_left,
                         int32_t output_index,
                         uintptr_t node_ptr_right,
                         int32_t input_index)
    {
        return _model.DisconnectNodes(node_ptr_left,
                                      output_index,
                                      node_ptr_right,
                                      input_index); // Disconnect the nodes in the model
    }
    void SetNodeInfo(uintptr_t node_ptr, std::string info)
    {
        _model.SetNodeInfo(node_ptr, info); // Set the node info in the model
    }
    auto CreateAnimation(uintptr_t node_ptr) -> uintptr_t
    {
        return _model.CreateAnimation(node_ptr);
    }
    void RemoveAnimation(uintptr_t animation_ptr) { _model.RemoveAnimation(animation_ptr); }
    auto AddPropertyTrack(uintptr_t animation_ptr,
                          std::string property_name,
                          std::string keyframes_json = {}) -> uintptr_t
    {
        return _model.AddPropertyTrack(animation_ptr, property_name, keyframes_json);
    }
    void AddKeyframe(uintptr_t track_ptr, std::string keyframes_json)
    {
        _model.AddKeyframe(track_ptr, keyframes_json);
    }
    void Play()
    {
        _model.Play();
        SetTransportState(TransportState::Playing);
    }
    void Stop()
    {
        _model.Stop();
        SetTransportState(TransportState::Paused, "Playback paused");
    }
    void ShowOpenProjectDialog()
    {
        std::vector<std::string> filters{ "*.vortex", "*.json", "*.*" };
        const auto request_id = _active_request_id;

        _ui_app.ShowOpenFileDialog(filters, [this, request_id](std::vector<std::filesystem::path> paths) {
            if (!paths.empty()) {
                SendUIReturnForRequest(request_id, paths.front().string());
            } else {
                SendUIReturnForRequest(request_id);
            }
        });
    }
    void ShowSelectFolderDialog()
    {
        const auto request_id = _active_request_id;

        _ui_app.ShowSelectFolderDialog([this, request_id](std::vector<std::filesystem::path> paths) {
            if (!paths.empty()) {
                SendUIReturnForRequest(request_id, paths.front().string());
            } else {
                SendUIReturnForRequest(request_id);
            }
        });
    }
    bool SaveProject(std::string path, std::string snapshot_json)
    {
        if (path.empty()) {
            vortex::error("SaveProjectAsync called without a valid path");
            return false;
        }

        auto parsed_snapshot = ParseJson(snapshot_json, path);
        if (!parsed_snapshot) {
            return false;
        }

        std::filesystem::path target_path(path);
        if (std::filesystem::exists(target_path) && std::filesystem::is_directory(target_path)) {
            vortex::error("SaveProjectAsync target {} is a directory; expected a file path",
                          target_path.string());
            return false;
        }

        if (!WriteSnapshotToDisk(target_path, snapshot_json)) {
            return false;
        }

        SetActiveProject(target_path, std::move(*parsed_snapshot), false);
        if (_active_project) {
            MaybeCreateBackup(*_active_project, snapshot_json);
            EmitProjectPersistedEvent(*_active_project, "legacy-save", _active_project->last_hash, MakeIsoTimestamp());
        }
        vortex::info("Project saved to {}", target_path.string());
        return true;
    }

    bool ApplyProjectPatch(std::string path, std::string patch_json)
    {
        if (path.empty()) {
            vortex::error("ApplyProjectPatchAsync called without a valid path");
            return false;
        }

        std::filesystem::path target_path(path);
        if (!EnsureActiveProject(target_path)) {
            vortex::error("ApplyProjectPatchAsync could not resolve target {}", path);
            return false;
        }

        auto patch_payload = ParseJson(patch_json, "ApplyProjectPatch payload");
        if (!patch_payload || !patch_payload->is_array()) {
            vortex::error("ApplyProjectPatch payload must be a JSON array");
            return false;
        }

        try {
            _active_project->snapshot = _active_project->snapshot.patch(*patch_payload);
            MarkProjectDirty();
        } catch (const std::exception& ex) {
            vortex::error("Failed to apply project patch: {}", ex.what());
            return false;
        }

        return true;
    }

    bool PersistProject(std::string path)
    {
        if (path.empty()) {
            vortex::error("PersistProjectAsync called without a valid path");
            return false;
        }

        std::filesystem::path target_path(path);
        if (!EnsureActiveProject(target_path)) {
            vortex::error("PersistProjectAsync could not resolve target {}", path);
            return false;
        }

        if (!PersistActiveProject("manual-save", true)) {
            return false;
        }

        vortex::info("Project persisted to {}", _active_project->path.string());
        return true;
    }

    bool ResetProjectState(std::string path, std::string snapshot_json)
    {
        if (path.empty()) {
            vortex::error("ResetProjectStateAsync called without a valid path");
            return false;
        }

        auto snapshot = ParseJson(snapshot_json, "ResetProjectState payload");
        if (!snapshot) {
            return false;
        }

        std::filesystem::path target_path(path);
        SetActiveProject(target_path, std::move(*snapshot), true);
        return true;
    }

    bool SubmitProjectCommands(std::string path, std::string batch_json)
    {
        if (path.empty()) {
            vortex::error("SubmitProjectCommandsAsync called without a valid path");
            return false;
        }

        auto batch = ParseJson(batch_json, "SubmitProjectCommands payload");
        if (!batch || !batch->is_object()) {
            vortex::error("SubmitProjectCommands payload must be a JSON object");
            return false;
        }

        if (!batch->contains("commands") || !(*batch)["commands"].is_array()) {
            vortex::error("SubmitProjectCommands payload missing commands array");
            return false;
        }

        std::filesystem::path target_path(path);
        if (!EnsureActiveProject(target_path)) {
            vortex::error("SubmitProjectCommandsAsync could not resolve target {}", path);
            return false;
        }

        auto base_revision = batch->contains("baseRevision") && (*batch)["baseRevision"].is_string()
                ? (*batch)["baseRevision"].get<std::string>()
                : std::string();
        if (!base_revision.empty() && _active_project &&
            !_active_project->last_hash.empty() && _active_project->last_hash != base_revision) {
            vortex::warn("SubmitProjectCommands baseRevision {} does not match active {}",
                         base_revision,
                         _active_project->last_hash);
        }

        bool mutated = false;
        json& snapshot = _active_project->snapshot;
        for (const auto& command : (*batch)["commands"]) {
            if (!command.is_object()) {
                vortex::error("SubmitProjectCommands encountered non-object command entry");
                return false;
            }

            const std::string kind = command.contains("kind") && command["kind"].is_string()
                                             ? command["kind"].get<std::string>()
                                             : std::string();
            auto parsed_kind = ParseCommandKind(kind);
            if (!parsed_kind) {
                vortex::error("SubmitProjectCommands received unsupported kind {}", kind);
                return false;
            }

            const json payload = command.contains("payload") ? command["payload"] : json::object();
            if (!payload.is_object()) {
                vortex::error("SubmitProjectCommands payload for {} must be an object", kind);
                return false;
            }

            if (!ApplyProjectCommand(*parsed_kind, payload, snapshot)) {
                vortex::error("SubmitProjectCommands failed while applying {}", kind);
                return false;
            }
            mutated = true;
        }

        if (mutated) {
            snapshot["updatedAt"] = MakeIsoTimestamp();
            MarkProjectDirty();
            if (!PersistActiveProject("command-batch", false)) {
                vortex::warn("SubmitProjectCommands was unable to persist changes immediately");
            }
        }

        return true;
    }

    bool ConfigureAutosave(bool enabled, double delay_ms)
    {
        if (!_active_project) {
            vortex::warn("ConfigureAutosave called without an active project");
            return false;
        }

        const auto delay = NormalizeAutosaveDelay(delay_ms);
        ApplyAutosavePolicy(*_active_project, enabled, delay);
        vortex::info("Autosave {} with delay {}ms",
                     enabled ? "enabled" : "disabled",
                     delay.count());
        return true;
    }

    CefRefPtr<CefDictionaryValue> OpenProject(std::string path)
    {
        if (path.empty()) {
            vortex::error("OpenProjectAsync called without a valid path");
            return nullptr;
        }

        std::filesystem::path project_path(path);
        auto snapshot = ReadSnapshotFromDisk(project_path);
        if (!snapshot) {
            return nullptr;
        }

        auto dto = SnapshotToProjectDTO(*snapshot, project_path.string());
        SetActiveProject(project_path, std::move(*snapshot), false);
        return JsonToDictionary(dto);
    }

    CefRefPtr<CefDictionaryValue> CreateProject(std::string payload_json)
    {
        if (payload_json.empty()) {
            vortex::error("CreateProjectAsync payload was empty");
            return nullptr;
        }

        auto payload = ParseJson(payload_json, "CreateProject payload");
        if (!payload) {
            return nullptr;
        }

        const std::string raw_location = payload->contains("location") && (*payload)["location"].is_string()
                                             ? (*payload)["location"].get<std::string>()
                                             : std::string();
        if (raw_location.empty()) {
            vortex::error("CreateProjectAsync missing target location");
            return nullptr;
        }

        const std::string requested_name = payload->contains("name") && (*payload)["name"].is_string()
                            ? (*payload)["name"].get<std::string>()
                            : std::string("Untitled");
        auto target_path = EnsureSnapshotPath(raw_location, requested_name);

        json snapshot = BuildSnapshotFromPayload(*payload, target_path.string());
        const std::string snapshot_json = snapshot.dump(2);

        if (!SaveProject(target_path.string(), snapshot_json)) {
            return nullptr;
        }

        vortex::info("Created new project at {}", target_path.string());
        auto dto = SnapshotToProjectDTO(snapshot, target_path.string());
        return JsonToDictionary(dto);
    }

private:
    // Thunk for node update observer
    static void OnNodeUpdateThunk(void* observer,
                                  uintptr_t node,
                                  uint32_t property_index,
                                  std::string_view value)
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
        _ui_app.SendUIMessage(u"node_update",
                              std::bit_cast<double>(node),
                              static_cast<int32_t>(property_index),
                              std::string(value));
    }

public:
    template<typename... Args>
    void HandleUIReturn(Args&&... args)
    {
        SendUIReturnForRequest(_active_request_id, std::forward<Args>(args)...);
    }

private:
    template<typename... Args>
    void SendUIReturnForRequest(const std::optional<uint64_t>& request_id, Args&&... args)
    {
        if (request_id) {
            _ui_app.SendRoutedUIReturn(*request_id, std::forward<Args>(args)...);
        } else {
            _ui_app.SendUIReturn(std::forward<Args>(args)...);
        }
    }

public:
    static std::optional<json> ParseJson(std::string_view source, std::string_view purpose)
    {
        if (source.empty()) {
            vortex::error("{} was empty", purpose);
            return std::nullopt;
        }

        try {
            return json::parse(std::string(source));
        } catch (const std::exception& ex) {
            vortex::error("Failed to parse {}: {}", purpose, ex.what());
        }
        return std::nullopt;
    }

    static std::filesystem::file_time_type SafeLastWriteTime(const std::filesystem::path& target)
    {
        std::error_code ec;
        const auto ts = std::filesystem::last_write_time(target, ec);
        if (ec) {
            return std::filesystem::file_time_type::min();
        }
        return ts;
    }

    static std::string SerializeSnapshotCompact(const json& snapshot)
    {
        return snapshot.dump();
    }

    static std::string HashSerializedSnapshot(std::string_view serialized)
    {
        const std::size_t value = std::hash<std::string_view>{}(serialized);
        std::ostringstream oss;
        oss << std::hex << value;
        return oss.str();
    }

    static std::optional<json> ReadSnapshotFromDisk(const std::filesystem::path& project_path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(project_path, ec) ||
            !std::filesystem::is_regular_file(project_path, ec)) {
            vortex::error("Project file {} does not exist or is not a file",
                          project_path.string());
            return std::nullopt;
        }

        std::ifstream stream(project_path, std::ios::binary);
        if (!stream.is_open()) {
            vortex::error("Failed to open project file {}", project_path.string());
            return std::nullopt;
        }

        std::string contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        stream.close();

        return ParseJson(contents, project_path.string());
    }

    static std::string NormalizePathString(std::filesystem::path path)
    {
        path = path.lexically_normal();
        path.make_preferred();
        return path.string();
    }

    static std::string FormatIsoTimestamp(std::chrono::system_clock::time_point tp)
    {
        const auto timer = std::chrono::system_clock::to_time_t(tp);
        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &timer);
#else
        gmtime_r(&timer, &utc);
#endif
        char buffer[32];
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0) {
            return "1970-01-01T00:00:00Z";
        }
        return std::string(buffer);
    }

    static std::string MakeIsoTimestamp()
    {
        return FormatIsoTimestamp(std::chrono::system_clock::now());
    }

    static std::string FileTimeToIsoString(std::filesystem::file_time_type file_time)
    {
        if (file_time == std::filesystem::file_time_type::min()) {
            return "1970-01-01T00:00:00Z";
        }

        const auto system_ts = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                file_time - decltype(file_time)::clock::now() + std::chrono::system_clock::now());
        return FormatIsoTimestamp(system_ts);
    }

    static std::string SafeFileName(std::string_view name)
    {
        std::string result;
        result.reserve(name.size());
        for (char ch : name) {
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_' || ch == '.') {
                result.push_back(ch);
            } else if (!result.empty() && result.back() != '-') {
                result.push_back('-');
            }
        }

        if (result.empty()) {
            result = "project";
        }
        return result;
    }

    static std::filesystem::path EnsureSnapshotPath(std::string_view raw_path,
                                                    std::string_view fallback_name)
    {
        std::string trimmed(raw_path);
        auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());

        while (!trimmed.empty() && (trimmed.back() == '\\' || trimmed.back() == '/')) {
            trimmed.pop_back();
        }

        const std::string safe_name = SafeFileName(fallback_name);
        if (trimmed.empty()) {
            return std::filesystem::path(safe_name + ".vortex");
        }

        std::filesystem::path normalized(trimmed);
        if (normalized.has_extension()) {
            normalized.replace_extension(".vortex");
            return normalized;
        }

        return normalized / std::format("{}.vortex", safe_name);
    }

    static json BuildSnapshotFromPayload(const json& payload, const std::string& path)
    {
        const std::string timestamp = MakeIsoTimestamp();
        const std::string name = payload.contains("name") && payload["name"].is_string()
                                      ? payload["name"].get<std::string>()
                                      : std::string("Untitled");
        const std::string version = payload.contains("version") && payload["version"].is_string()
                                            ? payload["version"].get<std::string>()
                                            : std::string("1.0");
        json snapshot;
        snapshot["path"] = path;
        snapshot["meta"] = {
            { "name", name.empty() ? "Untitled" : name },
            { "version", version },
            { "template", payload.contains("template") ? payload["template"] : json(nullptr) },
            { "lastOpened", timestamp }
        };
        snapshot["settings"] = {
            { "width", payload.contains("width") && payload["width"].is_number() ? payload["width"].get<int>() : 1920 },
            { "height", payload.contains("height") && payload["height"].is_number() ? payload["height"].get<int>() : 1080 },
            { "fps", payload.contains("fps") && payload["fps"].is_number() ? payload["fps"].get<int>() : 60 },
            { "colorSpace", payload.contains("colorSpace") && payload["colorSpace"].is_string() ? payload["colorSpace"].get<std::string>() : std::string("Rec.709") }
        };
        snapshot["graph"] = {
            { "nodes", json::array() },
            { "edges", json::array() }
        };
        snapshot["updatedAt"] = timestamp;
        return snapshot;
    }

    static json BuildGraphDTO(const json& graph)
    {
        if (!graph.is_object()) {
            return json{ { "nodes", json::array() }, { "edges", json::array() } };
        }

        json nodes = json::array();
        if (graph.contains("nodes") && graph["nodes"].is_array()) {
            size_t index = 0;
            for (const auto& node : graph["nodes"]) {
                nodes.push_back(BuildNodeDTO(node, index++));
            }
        }

        json edges = json::array();
        if (graph.contains("edges") && graph["edges"].is_array()) {
            size_t index = 0;
            for (const auto& edge : graph["edges"]) {
                edges.push_back(BuildEdgeDTO(edge, index++));
            }
        }

        return json{ { "nodes", nodes }, { "edges", edges } };
    }

    static json BuildNodeDTO(const json& node, size_t index)
    {
        json params = json::object();
        if (node.contains("params") && node["params"].is_object()) {
            params = node["params"];
        } else if (node.contains("props") && node["props"].is_object()) {
            params = node["props"];
        }
        if (node.contains("label") && node["label"].is_string()) {
            params["label"] = node["label"].get<std::string>();
        }

        double x = 0.0;
        double y = 0.0;
        if (node.contains("pos") && node["pos"].is_array() && node["pos"].size() >= 2) {
            const auto& raw = node["pos"];
            x = raw[0].is_number() ? raw[0].get<double>() : 0.0;
            y = raw[1].is_number() ? raw[1].get<double>() : 0.0;
        } else if (node.contains("position") && node["position"].is_object()) {
            const auto& raw = node["position"];
            x = raw.contains("x") && raw["x"].is_number() ? raw["x"].get<double>() : 0.0;
            y = raw.contains("y") && raw["y"].is_number() ? raw["y"].get<double>() : 0.0;
        }

        json pos = json::array({ x, y });
        std::string id = node.contains("id") && node["id"].is_string() ? node["id"].get<std::string>() : std::format("node-{}", index);
        if (id.empty()) {
            id = std::format("node-{}", index);
        }
        std::string type = node.contains("type") && node["type"].is_string() ? node["type"].get<std::string>() : std::string("Node");

        json dto = {
            { "id", id },
            { "type", type.empty() ? "Node" : type },
            { "params", params },
            { "pos", pos }
        };

        if (node.contains("ptr") && node["ptr"].is_number()) {
            dto["ptr"] = node["ptr"].get<double>();
        }

        return dto;
    }

    static json BuildEdgeDTO(const json& edge, size_t index)
    {
        std::string id = edge.contains("id") && edge["id"].is_string() ? edge["id"].get<std::string>() : std::format("edge-{}", index);
        if (id.empty()) {
            id = std::format("edge-{}", index);
        }
        std::string from;
        if (edge.contains("from") && edge["from"].is_string()) {
            from = edge["from"].get<std::string>();
        } else if (edge.contains("source") && edge["source"].is_string()) {
            from = edge["source"].get<std::string>();
        }

        std::string to;
        if (edge.contains("to") && edge["to"].is_string()) {
            to = edge["to"].get<std::string>();
        } else if (edge.contains("target") && edge["target"].is_string()) {
            to = edge["target"].get<std::string>();
        }

        return json{ { "id", id }, { "from", from }, { "to", to } };
    }

    static json SnapshotToProjectDTO(const json& snapshot, const std::string& resolved_path)
    {
        const json meta = snapshot.contains("meta") && snapshot["meta"].is_object() ? snapshot["meta"] : json::object();
        const json settings = snapshot.contains("settings") && snapshot["settings"].is_object() ? snapshot["settings"] : json::object();
        json dto;

        if (meta.contains("version") && meta["version"].is_string()) {
            dto["version"] = meta["version"].get<std::string>();
        } else if (snapshot.contains("version") && snapshot["version"].is_string()) {
            dto["version"] = snapshot["version"].get<std::string>();
        } else {
            dto["version"] = "1.0";
        }

        std::filesystem::path fallback(resolved_path);
        std::string name = meta.contains("name") && meta["name"].is_string() ? meta["name"].get<std::string>() : fallback.stem().string();
        if (name.empty()) {
            name = "Untitled";
        }
        dto["name"] = name;
        dto["path"] = resolved_path;

        if (meta.contains("template") && (meta["template"].is_string() || meta["template"].is_null())) {
            dto["template"] = meta["template"];
        } else if (snapshot.contains("template") && (snapshot["template"].is_string() || snapshot["template"].is_null())) {
            dto["template"] = snapshot["template"];
        } else {
            dto["template"] = nullptr;
        }

        dto["settings"] = {
            { "width", settings.contains("width") && settings["width"].is_number() ? settings["width"].get<int>() : 1920 },
            { "height", settings.contains("height") && settings["height"].is_number() ? settings["height"].get<int>() : 1080 },
            { "fps", settings.contains("fps") && settings["fps"].is_number() ? settings["fps"].get<int>() : 60 },
            { "colorSpace", settings.contains("colorSpace") && settings["colorSpace"].is_string() ? settings["colorSpace"].get<std::string>() : std::string("Rec.709") }
        };

        if (snapshot.contains("assets") && snapshot["assets"].is_array()) {
            dto["assets"] = snapshot["assets"];
        } else {
            dto["assets"] = json::array();
        }

        const json graph = snapshot.contains("graph") && snapshot["graph"].is_object() ? snapshot["graph"] : json::object();
        dto["graph"] = BuildGraphDTO(graph);

        return dto;
    }

    struct ResolvedNodeRef {
        uintptr_t ptr{ 0 };
        std::string id;
        json* node{ nullptr };
    };

    static std::string TrimCopy(std::string_view value)
    {
        auto begin = value.find_first_not_of(" \t\r\n");
        auto end = value.find_last_not_of(" \t\r\n");
        if (begin == std::string_view::npos) {
            return std::string();
        }
        return std::string(value.substr(begin, end - begin + 1));
    }

    static json& EnsureGraphObject(json& snapshot)
    {
        if (!snapshot.contains("graph") || !snapshot["graph"].is_object()) {
            snapshot["graph"] = json::object();
        }
        json& graph = snapshot["graph"];
        if (!graph.contains("nodes") || !graph["nodes"].is_array()) {
            graph["nodes"] = json::array();
        }
        if (!graph.contains("edges") || !graph["edges"].is_array()) {
            graph["edges"] = json::array();
        }
        return graph;
    }

    static std::optional<uintptr_t> ExtractPointer(const json& handle)
    {
        if (!handle.contains("ptr")) {
            return std::nullopt;
        }
        const auto& ptr_value = handle["ptr"];
        if (!ptr_value.is_number()) {
            return std::nullopt;
        }
        auto numeric = ptr_value.get<double>();
        if (numeric <= 0.0) {
            return std::nullopt;
        }
        return static_cast<uintptr_t>(numeric);
    }

    static std::optional<std::string> ExtractHandleId(const json& handle)
    {
        if (!handle.contains("id") || !handle["id"].is_string()) {
            return std::nullopt;
        }
        auto trimmed = TrimCopy(handle["id"].get<std::string>());
        if (trimmed.empty()) {
            return std::nullopt;
        }
        return trimmed;
    }

    static std::optional<ResolvedNodeRef> ResolveNodeHandle(json& snapshot, const json& handle)
    {
        if (!handle.is_object()) {
            return std::nullopt;
        }

        json& graph = EnsureGraphObject(snapshot);
        auto ptr = ExtractPointer(handle);
        auto id = ExtractHandleId(handle);
        json& nodes = graph["nodes"];

        for (auto& node : nodes) {
            if (!node.is_object()) {
                continue;
            }
            bool matches = false;
            if (ptr && node.contains("ptr") && node["ptr"].is_number()) {
                const auto node_ptr = static_cast<uintptr_t>(node["ptr"].get<double>());
                matches = node_ptr == *ptr;
            }
            if (!matches && id && node.contains("id") && node["id"].is_string()) {
                matches = TrimCopy(node["id"].get<std::string>()) == *id;
            }
            if (matches) {
                ResolvedNodeRef ref;
                if (node.contains("ptr") && node["ptr"].is_number()) {
                    ref.ptr = static_cast<uintptr_t>(node["ptr"].get<double>());
                } else if (ptr) {
                    ref.ptr = *ptr;
                }
                if (node.contains("id") && node["id"].is_string()) {
                    ref.id = TrimCopy(node["id"].get<std::string>());
                } else if (id) {
                    ref.id = *id;
                }
                ref.node = &node;
                return ref;
            }
        }

        return std::nullopt;
    }

    static std::optional<ResolvedNodeRef> ResolveNodeById(json& snapshot, std::string_view id)
    {
        json handle = json::object({ { "id", std::string(id) } });
        return ResolveNodeHandle(snapshot, handle);
    }

    static std::optional<vortex::commands::CommandKind> ParseCommandKind(std::string_view kind)
    {
        using vortex::commands::CommandKind;
        if (kind == vortex::commands::ToString(CommandKind::GraphNodeCreate)) {
            return CommandKind::GraphNodeCreate;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphNodeRemove)) {
            return CommandKind::GraphNodeRemove;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphNodePosition)) {
            return CommandKind::GraphNodePosition;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphNodeLabel)) {
            return CommandKind::GraphNodeLabel;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphNodeProps)) {
            return CommandKind::GraphNodeProps;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphEdgeConnect)) {
            return CommandKind::GraphEdgeConnect;
        }
        if (kind == vortex::commands::ToString(CommandKind::GraphEdgeDisconnect)) {
            return CommandKind::GraphEdgeDisconnect;
        }
        if (kind == vortex::commands::ToString(CommandKind::ProjectSettings)) {
            return CommandKind::ProjectSettings;
        }
        if (kind == vortex::commands::ToString(CommandKind::ProjectMeta)) {
            return CommandKind::ProjectMeta;
        }
        return std::nullopt;
    }

    static std::string MakeEdgeIdentifier(const std::string& source_id,
                                          int32_t source_slot,
                                          const std::string& target_id,
                                          int32_t target_slot)
    {
        return std::format("{}:{}->{}:{}", source_id, source_slot, target_id, target_slot);
    }

    bool ApplyProjectCommand(vortex::commands::CommandKind kind, const json& payload, json& snapshot)
    {
        using vortex::commands::CommandKind;
        switch (kind) {
            case CommandKind::GraphNodeCreate:
                return ApplyGraphNodeCreate(payload, snapshot);
            case CommandKind::GraphNodeRemove:
                return ApplyGraphNodeRemove(payload, snapshot);
            case CommandKind::GraphNodePosition:
                return ApplyGraphNodePosition(payload, snapshot);
            case CommandKind::GraphNodeLabel:
                return ApplyGraphNodeLabel(payload, snapshot);
            case CommandKind::GraphNodeProps:
                return ApplyGraphNodeProps(payload, snapshot);
            case CommandKind::GraphEdgeConnect:
                return ApplyGraphEdgeConnect(payload, snapshot);
            case CommandKind::GraphEdgeDisconnect:
                return ApplyGraphEdgeDisconnect(payload, snapshot);
            case CommandKind::ProjectSettings:
                return ApplyProjectSettings(payload, snapshot);
            case CommandKind::ProjectMeta:
                return ApplyProjectMeta(payload, snapshot);
            default:
                return false;
        }
    }

    bool ApplyGraphNodeCreate(const json& payload, json& snapshot)
    {
        if (!payload.contains("type") || !payload["type"].is_string()) {
            vortex::error("graph.node.create missing type field");
            return false;
        }

        std::string type = TrimCopy(payload["type"].get<std::string>());
        if (type.empty()) {
            vortex::error("graph.node.create received empty type");
            return false;
        }

        std::string label;
        if (payload.contains("label") && payload["label"].is_string()) {
            label = TrimCopy(payload["label"].get<std::string>());
        }
        if (label.empty()) {
            label = type;
        }

        double x = 0.0;
        double y = 0.0;
        if (payload.contains("position") && payload["position"].is_object()) {
            const auto& pos = payload["position"];
            if (pos.contains("x") && pos["x"].is_number()) {
                x = pos["x"].get<double>();
            }
            if (pos.contains("y") && pos["y"].is_number()) {
                y = pos["y"].get<double>();
            }
        }

        json props = json::object();
        if (payload.contains("props") && payload["props"].is_object()) {
            props = payload["props"];
        }

        std::string client_id;
        if (payload.contains("clientNodeId") && payload["clientNodeId"].is_string()) {
            client_id = TrimCopy(payload["clientNodeId"].get<std::string>());
        }

        uintptr_t ptr = CreateNode(type);
        if (ptr == 0) {
            vortex::error("graph.node.create failed to allocate {}", type);
            return false;
        }

        std::string node_id = !client_id.empty() ? client_id : std::format("{}-{}", type, ptr);
        json& graph = EnsureGraphObject(snapshot);
        json node = json::object({
                { "id", node_id },
                { "type", type },
                { "label", label },
                { "position", json{ { "x", x }, { "y", y } } },
                { "ptr", static_cast<double>(ptr) },
                { "props", props }
        });
        EmitGraphNodeCreatedEvent(node, client_id, ptr);
        graph["nodes"].push_back(std::move(node));
        return true;
    }

    bool ApplyGraphNodeRemove(const json& payload, json& snapshot)
    {
        if (!payload.contains("target")) {
            vortex::error("graph.node.remove missing target");
            return false;
        }

        auto resolved = ResolveNodeHandle(snapshot, payload["target"]);
        if (!resolved) {
            vortex::error("graph.node.remove could not resolve target");
            return false;
        }

        if (resolved->ptr != 0) {
            _model.RemoveNode(resolved->ptr);
        }

        json& graph = EnsureGraphObject(snapshot);
        auto& nodes = graph["nodes"];
        for (auto it = nodes.begin(); it != nodes.end();) {
            bool matches = false;
            if (resolved->ptr != 0 && (*it).contains("ptr") && (*it)["ptr"].is_number()) {
                matches = static_cast<uintptr_t>((*it)["ptr"].get<double>()) == resolved->ptr;
            }
            if (!matches && (*it).contains("id") && (*it)["id"].is_string()) {
                matches = TrimCopy((*it)["id"].get<std::string>()) == resolved->id;
            }

            if (matches) {
                it = nodes.erase(it);
            } else {
                ++it;
            }
        }

        auto& edges = graph["edges"];
        for (auto it = edges.begin(); it != edges.end();) {
            if (!it->is_object()) {
                it = edges.erase(it);
                continue;
            }
            const std::string source = it->contains("source") && (*it)["source"].is_string()
                    ? TrimCopy((*it)["source"].get<std::string>())
                    : std::string();
            const std::string target = it->contains("target") && (*it)["target"].is_string()
                    ? TrimCopy((*it)["target"].get<std::string>())
                    : std::string();
            if (source == resolved->id || target == resolved->id) {
                it = edges.erase(it);
            } else {
                ++it;
            }
        }

        return true;
    }

    bool ApplyGraphNodePosition(const json& payload, json& snapshot)
    {
        if (!payload.contains("target") || !payload.contains("position")) {
            vortex::error("graph.node.position missing fields");
            return false;
        }

        auto resolved = ResolveNodeHandle(snapshot, payload["target"]);
        if (!resolved || !resolved->node) {
            vortex::error("graph.node.position could not resolve target");
            return false;
        }

        if (!payload["position"].is_object()) {
            vortex::error("graph.node.position expects object payload");
            return false;
        }

        const auto& position = payload["position"];
        double x = position.contains("x") && position["x"].is_number() ? position["x"].get<double>() : 0.0;
        double y = position.contains("y") && position["y"].is_number() ? position["y"].get<double>() : 0.0;
        (*resolved->node)["position"] = json{ { "x", x }, { "y", y } };
        return true;
    }

    bool ApplyGraphNodeLabel(const json& payload, json& snapshot)
    {
        if (!payload.contains("target") || !payload.contains("label") || !payload["label"].is_string()) {
            vortex::error("graph.node.label missing fields");
            return false;
        }

        auto resolved = ResolveNodeHandle(snapshot, payload["target"]);
        if (!resolved || !resolved->node) {
            vortex::error("graph.node.label could not resolve target");
            return false;
        }

        std::string label = TrimCopy(payload["label"].get<std::string>());
        if (label.empty()) {
            label = resolved->id;
        }
        (*resolved->node)["label"] = label;
        return true;
    }

    bool ApplyGraphNodeProps(const json& payload, json& snapshot)
    {
        if (!payload.contains("target") || !payload.contains("props")) {
            vortex::error("graph.node.props missing fields");
            return false;
        }

        if (!payload["props"].is_object()) {
            vortex::error("graph.node.props expects props object");
            return false;
        }

        auto resolved = ResolveNodeHandle(snapshot, payload["target"]);
        if (!resolved || !resolved->node) {
            vortex::error("graph.node.props could not resolve target");
            return false;
        }

        json& props = (*resolved->node)["props"];
        if (!props.is_object()) {
            props = json::object();
        }
        for (const auto& [key, value] : payload["props"].items()) {
            props[key] = value;
        }
        return true;
    }

    bool ApplyGraphEdgeConnect(const json& payload, json& snapshot)
    {
        if (!payload.contains("source") || !payload.contains("target")) {
            vortex::error("graph.edge.connect missing handles");
            return false;
        }

        auto source = ResolveNodeHandle(snapshot, payload["source"]);
        auto target = ResolveNodeHandle(snapshot, payload["target"]);
        if (!source || !target) {
            vortex::error("graph.edge.connect could not resolve nodes");
            return false;
        }

        const json& source_payload = payload["source"];
        const json& target_payload = payload["target"];
        const auto source_slot = source_payload.is_object() && source_payload.contains("slot") && source_payload["slot"].is_number()
            ? static_cast<int32_t>(source_payload["slot"].get<double>())
            : 0;
        const auto target_slot = target_payload.is_object() && target_payload.contains("slot") && target_payload["slot"].is_number()
            ? static_cast<int32_t>(target_payload["slot"].get<double>())
            : 0;

        if (source->ptr == 0 || target->ptr == 0) {
            vortex::error("graph.edge.connect requires resolved node pointers");
            return false;
        }

        if (!_model.ConnectNodes(source->ptr, source_slot, target->ptr, target_slot)) {
            vortex::error("graph.edge.connect engine call failed");
            return false;
        }

        std::string edge_id;
        if (payload.contains("edgeId") && payload["edgeId"].is_string()) {
            edge_id = TrimCopy(payload["edgeId"].get<std::string>());
        }
        if (edge_id.empty()) {
            edge_id = MakeEdgeIdentifier(source->id, source_slot, target->id, target_slot);
        }

        json& graph = EnsureGraphObject(snapshot);
        auto& edges = graph["edges"];
        bool exists = false;
        for (const auto& edge : edges) {
            if (!edge.is_object()) {
                continue;
            }
            if (edge.contains("id") && edge["id"].is_string() && TrimCopy(edge["id"].get<std::string>()) == edge_id) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            json edge = json::object({
                    { "id", edge_id },
                    { "source", source->id },
                    { "target", target->id },
                    { "animated", true },
                    { "sourceSlot", source_slot },
                    { "targetSlot", target_slot }
            });
            edges.push_back(std::move(edge));
        }

        return true;
    }

    bool ApplyGraphEdgeDisconnect(const json& payload, json& snapshot)
    {
        if (!payload.contains("edgeId") && !payload.contains("source") && !payload.contains("target")) {
            vortex::error("graph.edge.disconnect missing identifying fields");
            return false;
        }

        std::optional<std::string> edge_id;
        if (payload.contains("edgeId") && payload["edgeId"].is_string()) {
            auto trimmed = TrimCopy(payload["edgeId"].get<std::string>());
            if (!trimmed.empty()) {
                edge_id = trimmed;
            }
        }

        const bool has_source_handle = payload.contains("source");
        const bool has_target_handle = payload.contains("target");
        auto source = has_source_handle ? ResolveNodeHandle(snapshot, payload["source"]) : std::nullopt;
        auto target = has_target_handle ? ResolveNodeHandle(snapshot, payload["target"]) : std::nullopt;

        const json empty_handle = json::object();
        const json& source_payload = has_source_handle ? payload["source"] : empty_handle;
        const json& target_payload = has_target_handle ? payload["target"] : empty_handle;
        int32_t source_slot = source_payload.is_object() && source_payload.contains("slot") && source_payload["slot"].is_number()
            ? static_cast<int32_t>(source_payload["slot"].get<double>())
            : 0;
        int32_t target_slot = target_payload.is_object() && target_payload.contains("slot") && target_payload["slot"].is_number()
            ? static_cast<int32_t>(target_payload["slot"].get<double>())
            : 0;

        json& graph = EnsureGraphObject(snapshot);
        if ((!source || source->ptr == 0 || target->ptr == 0 || !target) && edge_id) {
            for (const auto& edge : graph["edges"]) {
                if (!edge.is_object()) {
                    continue;
                }
                if (!edge.contains("id") || !edge["id"].is_string()) {
                    continue;
                }
                if (TrimCopy(edge["id"].get<std::string>()) != *edge_id) {
                    continue;
                }

                if (!source && edge.contains("source") && edge["source"].is_string()) {
                    source = ResolveNodeById(snapshot, edge["source"].get<std::string>());
                }
                if (!target && edge.contains("target") && edge["target"].is_string()) {
                    target = ResolveNodeById(snapshot, edge["target"].get<std::string>());
                }
                if (edge.contains("sourceSlot") && edge["sourceSlot"].is_number()) {
                    source_slot = static_cast<int32_t>(edge["sourceSlot"].get<double>());
                }
                if (edge.contains("targetSlot") && edge["targetSlot"].is_number()) {
                    target_slot = static_cast<int32_t>(edge["targetSlot"].get<double>());
                }
                break;
            }
        }

        if (!source || !target || source->ptr == 0 || target->ptr == 0) {
            vortex::error("graph.edge.disconnect could not resolve both endpoints");
            return false;
        }

        _model.DisconnectNodes(source->ptr, source_slot, target->ptr, target_slot);

        auto& edges = graph["edges"];
        for (auto it = edges.begin(); it != edges.end();) {
            if (!it->is_object()) {
                it = edges.erase(it);
                continue;
            }

            const std::string current_id = it->contains("id") && (*it)["id"].is_string()
                    ? TrimCopy((*it)["id"].get<std::string>())
                    : std::string();
            const std::string current_source = it->contains("source") && (*it)["source"].is_string()
                    ? TrimCopy((*it)["source"].get<std::string>())
                    : std::string();
            const std::string current_target = it->contains("target") && (*it)["target"].is_string()
                    ? TrimCopy((*it)["target"].get<std::string>())
                    : std::string();
            const int32_t current_source_slot = it->contains("sourceSlot") && (*it)["sourceSlot"].is_number()
                    ? static_cast<int32_t>((*it)["sourceSlot"].get<double>())
                    : 0;
            const int32_t current_target_slot = it->contains("targetSlot") && (*it)["targetSlot"].is_number()
                    ? static_cast<int32_t>((*it)["targetSlot"].get<double>())
                    : 0;

            bool remove = false;
            if (edge_id && current_id == *edge_id) {
                remove = true;
            } else if (current_source == source->id && current_target == target->id &&
                       current_source_slot == source_slot && current_target_slot == target_slot) {
                remove = true;
            }

            if (remove) {
                it = edges.erase(it);
            } else {
                ++it;
            }
        }

        return true;
    }

    bool ApplyProjectSettings(const json& payload, json& snapshot)
    {
        if (!payload.contains("settings") || !payload["settings"].is_object()) {
            vortex::error("project.settings missing settings object");
            return false;
        }

        json& settings = snapshot["settings"];
        if (!settings.is_object()) {
            settings = json::object();
        }

        const auto& src = payload["settings"];
        if (src.contains("width") && src["width"].is_number()) {
            settings["width"] = src["width"].get<int>();
        }
        if (src.contains("height") && src["height"].is_number()) {
            settings["height"] = src["height"].get<int>();
        }
        if (src.contains("fps") && src["fps"].is_number()) {
            settings["fps"] = src["fps"].get<int>();
        }
        if (src.contains("colorSpace") && (src["colorSpace"].is_string() || src["colorSpace"].is_null())) {
            settings["colorSpace"] = src["colorSpace"];
        }
        return true;
    }

    bool ApplyProjectMeta(const json& payload, json& snapshot)
    {
        if (!payload.contains("meta") || !payload["meta"].is_object()) {
            vortex::error("project.meta missing meta object");
            return false;
        }

        json& meta = snapshot["meta"];
        if (!meta.is_object()) {
            meta = json::object();
        }

        const auto& src = payload["meta"];
        if (src.contains("name") && src["name"].is_string()) {
            meta["name"] = TrimCopy(src["name"].get<std::string>());
        }
        if (src.contains("version") && src["version"].is_string()) {
            meta["version"] = TrimCopy(src["version"].get<std::string>());
        }
        if (src.contains("template") && (src["template"].is_string() || src["template"].is_null())) {
            meta["template"] = src["template"];
        }
        return true;
    }

    static CefRefPtr<CefDictionaryValue> JsonToDictionary(const json& data)
    {
        const std::string serialized = data.dump();
        CefRefPtr<CefValue> value = CefParseJSON(serialized, JSON_PARSER_RFC);
        if (!value || value->GetType() != VTYPE_DICTIONARY) {
            vortex::error("Failed to convert project data into a CefDictionaryValue");
            return nullptr;
        }
        return value->GetDictionary();
    }

private:
    static constexpr std::chrono::milliseconds kDefaultAutosaveDelay{ 1200 };
    static constexpr std::chrono::milliseconds kMinAutosaveDelay{ 250 };
    static constexpr std::chrono::milliseconds kMaxAutosaveDelay{ 15000 };
    static constexpr std::chrono::seconds kSelfPersistGrace{ 2 };
    static constexpr std::size_t kMaxBackups = 10;

    struct ProjectDocument {
        std::filesystem::path path;
        std::string normalized;
        json snapshot;
        bool dirty{ false };
        bool autosave_enabled{ true };
        std::chrono::milliseconds autosave_delay{ kDefaultAutosaveDelay };
        Clock::time_point next_autosave;
        Clock::time_point last_persist;
        std::filesystem::file_time_type disk_timestamp{};
        std::string last_hash;
    };

    void SetActiveProject(std::filesystem::path path, json snapshot, bool dirty)
    {
        if (!snapshot.contains("path") || !snapshot["path"].is_string()) {
            snapshot["path"] = path.string();
        }

        ProjectDocument document;
        document.path = std::move(path);
        document.normalized = NormalizePathString(document.path);
        document.snapshot = std::move(snapshot);
        document.dirty = dirty;
        ApplyAutosavePolicy(document, true, kDefaultAutosaveDelay);
        document.last_persist = Clock::now();
        const std::string compact = SerializeSnapshotCompact(document.snapshot);
        document.last_hash = HashSerializedSnapshot(compact);
        document.disk_timestamp = SafeLastWriteTime(document.path);
        document.next_autosave = dirty && document.autosave_enabled ? Clock::now() + document.autosave_delay
                                                                    : Clock::time_point{};
        _active_project = std::move(document);
        SetTransportState(TransportState::Ready);
    }

    void MarkProjectDirty()
    {
        if (!_active_project) {
            return;
        }

        _active_project->dirty = true;
        if (_active_project->autosave_enabled) {
            _active_project->next_autosave = Clock::now() + _active_project->autosave_delay;
        }
    }

    bool EnsureActiveProject(const std::filesystem::path& desired_path)
    {
        const std::string normalized = NormalizePathString(desired_path);
        if (_active_project && _active_project->normalized == normalized) {
            return true;
        }

        auto snapshot = ReadSnapshotFromDisk(desired_path);
        if (!snapshot) {
            return false;
        }

        SetActiveProject(desired_path, std::move(*snapshot), false);
        return true;
    }

    bool WriteSnapshotToDisk(const std::filesystem::path& target_path, std::string_view serialized)
    {
        if (target_path.empty()) {
            vortex::error("WriteSnapshotToDisk called without a valid target");
            return false;
        }

        const auto parent = target_path.parent_path();
        std::error_code ec;
        if (!parent.empty() && !std::filesystem::exists(parent) &&
            !std::filesystem::create_directories(parent, ec)) {
            vortex::error("Failed to create project directory {}: {}",
                          parent.string(),
                          ec.message());
            return false;
        }

        std::filesystem::path temp_path = target_path;
        temp_path += ".tmp";

        {
            std::ofstream stream(temp_path, std::ios::binary | std::ios::trunc);
            if (!stream.is_open()) {
                vortex::error("Failed to open temporary project file {}", temp_path.string());
                return false;
            }
            stream.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
            stream.flush();
            if (!stream.good()) {
                vortex::error("Failed to write snapshot to {}", temp_path.string());
                stream.close();
                std::filesystem::remove(temp_path, ec);
                return false;
            }
        }

        std::filesystem::rename(temp_path, target_path, ec);
        if (ec) {
            std::error_code copy_ec;
            std::filesystem::copy_file(temp_path,
                                       target_path,
                                       std::filesystem::copy_options::overwrite_existing,
                                       copy_ec);
            std::error_code cleanup_ec;
            std::filesystem::remove(temp_path, cleanup_ec);
            if (copy_ec) {
                vortex::error("Failed to finalize project save to {}: {}",
                              target_path.string(),
                              copy_ec.message());
                return false;
            }
        }

        return true;
    }

    void TickProjectPersistence()
    {
        if (!_active_project) {
            return;
        }

        auto& document = *_active_project;
        if (document.dirty && document.autosave_enabled) {
            const auto now = Clock::now();
            if (document.next_autosave == Clock::time_point{}) {
                document.next_autosave = now + document.autosave_delay;
            } else if (now >= document.next_autosave) {
                if (PersistActiveProject("autosave", true)) {
                    document.next_autosave = now + document.autosave_delay;
                }
            }
        }

        MonitorExternalProjectChanges(document);
    }

    void MonitorExternalProjectChanges(ProjectDocument& document)
    {
        if (document.path.empty()) {
            return;
        }

        const auto current_timestamp = SafeLastWriteTime(document.path);
        if (current_timestamp == std::filesystem::file_time_type::min()) {
            return;
        }

        if (document.disk_timestamp == current_timestamp) {
            return;
        }

        const auto now = Clock::now();
        const bool within_grace = (now - document.last_persist) <= kSelfPersistGrace;
        document.disk_timestamp = current_timestamp;
        if (within_grace) {
            return;
        }

        const std::string changed_at = FileTimeToIsoString(current_timestamp);
        EmitProjectExternalChangeEvent(document, changed_at);
    }

    bool PersistActiveProject(std::string_view reason, bool create_backup)
    {
        if (!_active_project) {
            return false;
        }

        auto& document = *_active_project;
        const std::string compact = SerializeSnapshotCompact(document.snapshot);
        const std::string pretty = document.snapshot.dump(2);
        if (!WriteSnapshotToDisk(document.path, pretty)) {
            return false;
        }

        document.dirty = false;
        document.last_persist = Clock::now();
        document.last_hash = HashSerializedSnapshot(compact);
        document.next_autosave = Clock::time_point{};
        document.disk_timestamp = SafeLastWriteTime(document.path);

        if (create_backup) {
            MaybeCreateBackup(document, pretty);
        }

        EmitProjectPersistedEvent(document, reason, document.last_hash, MakeIsoTimestamp());
        return true;
    }

    void EmitProjectPersistedEvent(const ProjectDocument& document,
                                   std::string_view reason,
                                   std::string_view hash,
                                   std::string_view saved_at)
    {
        _ui_app.SendUIMessage(u"project_persisted",
                              document.path.string(),
                              std::string(saved_at),
                              std::string(hash),
                              std::string(reason));
    }

    void EmitProjectExternalChangeEvent(const ProjectDocument& document, std::string_view changed_at)
    {
        _ui_app.SendUIMessage(u"project_external_change",
                              document.path.string(),
                              std::string(changed_at));
    }

    void MaybeCreateBackup(const ProjectDocument& document, std::string_view serialized)
    {
        const auto backup_dir = ResolveBackupDirectory(document);
        std::error_code ec;
        std::filesystem::create_directories(backup_dir, ec);
        if (ec) {
            vortex::warn("Failed to prepare backup directory {}: {}", backup_dir.string(), ec.message());
            return;
        }

        std::string timestamp = MakeIsoTimestamp();
        std::replace(timestamp.begin(), timestamp.end(), ':', '-');
        std::string safe_name = SafeFileName(document.path.stem().string());
        if (safe_name.empty()) {
            safe_name = "project";
        }
        const auto backup_path = backup_dir / std::format("{}-{}.vortex", timestamp, safe_name);

        std::ofstream stream(backup_path, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            vortex::warn("Failed to write backup {}", backup_path.string());
            return;
        }

        stream.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
        stream.close();

        PruneBackups(backup_dir, kMaxBackups);
    }

    std::filesystem::path ResolveBackupDirectory(const ProjectDocument& document) const
    {
        auto dir = std::filesystem::path("backups");
        dir /= document.path.stem();
        return dir;
    }

    void PruneBackups(const std::filesystem::path& directory, std::size_t max_files)
    {
        std::error_code ec;
        if (!std::filesystem::exists(directory, ec)) {
            return;
        }

        std::vector<std::filesystem::directory_entry> entries;
        std::filesystem::directory_iterator end;
        for (std::filesystem::directory_iterator it(directory, ec); !ec && it != end; ++it) {
            if (it->is_regular_file()) {
                entries.push_back(*it);
            }
        }
        if (ec) {
            return;
        }

        if (entries.size() <= max_files) {
            return;
        }

        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            std::error_code lhs_ec;
            std::error_code rhs_ec;
            const auto lhs_time = lhs.last_write_time(lhs_ec);
            const auto rhs_time = rhs.last_write_time(rhs_ec);
            if (lhs_ec || rhs_ec) {
                return lhs.path() < rhs.path();
            }
            return lhs_time > rhs_time;
        });

        for (std::size_t i = max_files; i < entries.size(); ++i) {
            std::filesystem::remove(entries[i].path(), ec);
        }
    }

    static std::chrono::milliseconds ClampAutosaveDelay(std::chrono::milliseconds delay)
    {
        const auto min = kMinAutosaveDelay.count();
        const auto max = kMaxAutosaveDelay.count();
        auto value = delay.count();
        if (value <= 0) {
            return kDefaultAutosaveDelay;
        }
        value = std::clamp(value, min, max);
        return std::chrono::milliseconds(value);
    }

    static std::chrono::milliseconds NormalizeAutosaveDelay(double delay_ms)
    {
        if (!std::isfinite(delay_ms) || delay_ms <= 0.0) {
            return kDefaultAutosaveDelay;
        }
        const auto converted = static_cast<long long>(std::llround(delay_ms));
        return ClampAutosaveDelay(std::chrono::milliseconds(converted));
    }

    void ApplyAutosavePolicy(ProjectDocument& document,
                             bool enabled,
                             std::chrono::milliseconds delay)
    {
        document.autosave_enabled = enabled;
        document.autosave_delay = ClampAutosaveDelay(delay);
        if (document.autosave_enabled && document.dirty) {
            document.next_autosave = Clock::now() + document.autosave_delay;
        } else {
            document.next_autosave = Clock::time_point{};
        }
    }

    vortex::NDILibrary _ndi;
    vortex::ui::SDLLibrary _sdl;

    vortex::Graphics _gfx;
    struct MessageMetadata {
        MessageHanlderDispatch dispatch;
        std::optional<uint64_t> request_id;
    };

    dro::SPSCQueue<CefRefPtr<CefProcessMessage>, 64> _message_queue; ///< Queue for messages from
                                                                     ///< the UI
    std::unordered_map<CefProcessMessage*, MessageMetadata> _pending_message_meta;

    // CEF client for UI
    vortex::ui::UIApp _ui_app;
    vortex::LazyToken _lazy_token; ///< Lazy token for removing lazy data before graphics shutdown
    vortex::graph::GraphModel _model; ///< Model containing nodes and outputs

    // Message handlers map - this should be a simple map lookup as these are
    // used in hot code, so it should be fast
    std::unordered_map<std::u16string_view, MessageHanlderDispatch> _message_handlers_disp{
        // Coroutines
        {      u"GetNodeTypesAsync",          ui::MessageDispatch<&App::GetNodeTypes>::Dispatch },
        {        u"CreateNodeAsync",            ui::MessageDispatch<&App::CreateNode>::Dispatch },
        { u"GetNodePropertiesAsync",     ui::MessageDispatch<&App::GetNodeProperties>::Dispatch },
        {   u"CreateAnimationAsync",       ui::MessageDispatch<&App::CreateAnimation>::Dispatch },
        {  u"AddPropertyTrackAsync",      ui::MessageDispatch<&App::AddPropertyTrack>::Dispatch },
        {      u"ConnectNodesAsync",          ui::MessageDispatch<&App::ConnectNodes>::Dispatch },
        {     u"OpenProjectAsync",        ui::MessageDispatch<&App::OpenProject>::Dispatch },
        {    u"CreateProjectAsync",      ui::MessageDispatch<&App::CreateProject>::Dispatch },
        {      u"SaveProjectAsync",          ui::MessageDispatch<&App::SaveProject>::Dispatch },
        { u"ApplyProjectPatchAsync", ui::MessageDispatch<&App::ApplyProjectPatch>::Dispatch },
        {   u"PersistProjectAsync",    ui::MessageDispatch<&App::PersistProject>::Dispatch },
        { u"ResetProjectStateAsync", ui::MessageDispatch<&App::ResetProjectState>::Dispatch },
        { u"SubmitProjectCommandsAsync", ui::MessageDispatch<&App::SubmitProjectCommands>::Dispatch },
        { u"ConfigureAutosaveAsync", ui::MessageDispatch<&App::ConfigureAutosave>::Dispatch },

        // Immediate calls (fire and forget)
        {             u"RemoveNode",            ui::MessageDispatch<&App::RemoveNode>::Dispatch },
        {        u"DisconnectNodes",       ui::MessageDispatch<&App::DisconnectNodes>::Dispatch },
        {            u"SetNodeInfo",           ui::MessageDispatch<&App::SetNodeInfo>::Dispatch },
        {        u"SetNodeProperty",       ui::MessageDispatch<&App::SetNodeProperty>::Dispatch },
        {  u"SetNodePropertyByName", ui::MessageDispatch<&App::SetNodePropertyByName>::Dispatch },
        {        u"RemoveAnimation",       ui::MessageDispatch<&App::RemoveAnimation>::Dispatch },
        {            u"AddKeyframe",           ui::MessageDispatch<&App::AddKeyframe>::Dispatch },
        {                   u"Play",                  ui::MessageDispatch<&App::Play>::Dispatch },
        {                   u"Stop",                  ui::MessageDispatch<&App::Stop>::Dispatch },
        { u"ShowOpenProjectDialogAsync", ui::MessageDispatch<&App::ShowOpenProjectDialog>::Dispatch },
        { u"ShowSelectFolderDialogAsync", ui::MessageDispatch<&App::ShowSelectFolderDialog>::Dispatch },
    };

private:
    std::optional<uint64_t> _active_request_id;
    const AppExitControl& _exit;
    std::optional<ProjectDocument> _active_project;
    Clock::time_point _next_transport_metrics_emit{};
    TransportState _transport_state{ TransportState::Ready };
};
} // namespace vortex