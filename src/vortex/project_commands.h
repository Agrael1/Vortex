#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

namespace vortex::commands {

struct Position2D {
    double x{ 0.0 };
    double y{ 0.0 };
};

struct NodeHandle {
    std::optional<uintptr_t> ptr;
    std::optional<std::string> id;

    [[nodiscard]] bool Empty() const noexcept
    {
        const bool has_ptr = ptr.has_value() && *ptr != 0;
        const bool has_id = id.has_value() && !id->empty();
        return !(has_ptr || has_id);
    }
};

struct NodeSlotHandle : NodeHandle {
    std::optional<int32_t> slot;
};

struct NodeCreateCommand {
    std::string type;
    std::optional<std::string> label;
    std::optional<Position2D> position;
    nlohmann::json props = nlohmann::json::object();
    std::optional<std::string> client_node_id;
};

struct NodeRemoveCommand {
    NodeHandle target;
};

struct NodePositionCommand {
    NodeHandle target;
    Position2D position;
};

struct NodeLabelCommand {
    NodeHandle target;
    std::string label;
};

struct NodePropsCommand {
    NodeHandle target;
    nlohmann::json props;
};

struct EdgeConnectCommand {
    NodeSlotHandle source;
    NodeSlotHandle target;
    std::optional<std::string> edge_id;
};

struct EdgeDisconnectCommand {
    std::optional<std::string> edge_id;
    NodeSlotHandle source;
    NodeSlotHandle target;
};

struct ProjectSettingsCommand {
    nlohmann::json settings;
};

struct ProjectMetaCommand {
    nlohmann::json meta;
};

enum class CommandKind {
    GraphNodeCreate,
    GraphNodeRemove,
    GraphNodePosition,
    GraphNodeLabel,
    GraphNodeProps,
    GraphEdgeConnect,
    GraphEdgeDisconnect,
    ProjectSettings,
    ProjectMeta,
};

using CommandPayload = std::variant<NodeCreateCommand,
                                    NodeRemoveCommand,
                                    NodePositionCommand,
                                    NodeLabelCommand,
                                    NodePropsCommand,
                                    EdgeConnectCommand,
                                    EdgeDisconnectCommand,
                                    ProjectSettingsCommand,
                                    ProjectMetaCommand>;

struct ProjectCommand {
    std::string id;
    CommandKind kind;
    CommandPayload payload;
    std::string timestamp;
};

struct ProjectCommandBatch {
    std::string project_path;
    std::optional<std::string> base_revision;
    std::optional<std::string> source;
    std::string issued_at;
    std::vector<ProjectCommand> commands;
};

constexpr std::string_view ToString(CommandKind kind) noexcept
{
    switch (kind) {
        case CommandKind::GraphNodeCreate:
            return "graph.node.create";
        case CommandKind::GraphNodeRemove:
            return "graph.node.remove";
        case CommandKind::GraphNodePosition:
            return "graph.node.position";
        case CommandKind::GraphNodeLabel:
            return "graph.node.label";
        case CommandKind::GraphNodeProps:
            return "graph.node.props";
        case CommandKind::GraphEdgeConnect:
            return "graph.edge.connect";
        case CommandKind::GraphEdgeDisconnect:
            return "graph.edge.disconnect";
        case CommandKind::ProjectSettings:
            return "project.settings";
        case CommandKind::ProjectMeta:
            return "project.meta";
        default:
            return "unknown";
    }
}

} // namespace vortex::commands
