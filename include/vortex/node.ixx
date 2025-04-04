module;
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <source_location>

export module vortex.node;
export import vortex.common;
import vortex.graphics;

// Comes from qlibs\reflect
struct REFLECT_STRUCT {
    void* MEMBER;
    enum class ENUM { VALUE };
}; // has to be in the global namespace

namespace reflect {
namespace detail {
template<auto... Vs>
[[nodiscard]] constexpr auto function_name() noexcept -> std::string_view
{
    return std::source_location::current().function_name();
}
template<class... Ts>
[[nodiscard]] constexpr auto function_name() noexcept -> std::string_view
{
    return std::source_location::current().function_name();
}
template<class T>
struct type_name_info {
    static constexpr auto name = function_name<int>();
    static constexpr auto begin = name.find("int");
    static constexpr auto end = name.substr(begin + std::size(std::string_view{ "int" }));
};

template<class T>
    requires std::is_class_v<T>
struct type_name_info<T> {
    static constexpr auto name = function_name<REFLECT_STRUCT>();
    static constexpr auto begin = name.find("REFLECT_STRUCT");
    static constexpr auto end = name.substr(begin + std::size(std::string_view{ "REFLECT_STRUCT" }));
};

} // namespace detail

template<class T>
[[nodiscard]] constexpr auto type_name() noexcept -> std::string_view
{
    using type_name_info = detail::type_name_info<std::remove_pointer_t<std::remove_cvref_t<T>>>;
    constexpr std::string_view function_name = detail::function_name<std::remove_pointer_t<std::remove_cvref_t<T>>>();
    constexpr std::string_view qualified_type_name = function_name.substr(type_name_info::begin, function_name.find(type_name_info::end) - type_name_info::begin);
    constexpr std::string_view tmp_type_name = qualified_type_name.substr(0, qualified_type_name.find_first_of("<", 1));
    constexpr std::string_view type_name = tmp_type_name.substr(tmp_type_name.find_last_of("::") + 1);
    static_assert(std::size(type_name) > 0u);
    if (std::is_constant_evaluated()) {
        return type_name;
    } else {
        return [&] {
            static constexpr const auto name = wis::fixed_string<std::size(type_name)>{ std::data(type_name) };
            return std::string_view{ name };
        }();
    }
}
}

namespace vortex {
export enum class NodeType {
    None,
    Input,
    Output,
    Filter,
};
export enum class NodeInput {
    INVALID = -1,
    OUTPUT_DESC = 0,

    USER_INPUT = 4096,
};

export struct NodeDesc {
    NodeInput input = NodeInput::INVALID;
    void* pNext = nullptr;
};

export struct OutputDesc {
    NodeInput input = NodeInput::OUTPUT_DESC;
    void* pNext = nullptr;

    wis::DataFormat format;
    wis::Size2D size;
    const char* name;
};

export struct INode {
    virtual ~INode() = default;
};
export struct IOutput : public vortex::INode {
};

export class NodeFactory
{
    // Use callback to create a node
    using CreateNodeCallback = std::unique_ptr<INode> (*)(const vortex::Graphics& gfx, NodeDesc* initializers);

public:
    NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

public:
    static void RegisterNode(std::string_view name, CreateNodeCallback callback)
    {
        node_creators[std::string(name)] = callback;
    }
    static std::unique_ptr<INode> CreateNode(std::string_view name, const vortex::Graphics& gfx, NodeDesc* initializers)
    {
        auto it = node_creators.find(name);
        if (it != node_creators.end()) {
            return it->second(gfx, initializers);
        }
        return nullptr;
    }

private:
    static inline std::unordered_map<std::string, CreateNodeCallback, vortex::string_hash, vortex::string_equal> node_creators;
};

export template<typename CRTP, typename Base>
struct NodeImpl : public Base {
    using Base::Base;

public:
    static constexpr std::string_view name = reflect::type_name<CRTP>();
    static void RegisterNode()
    {
        auto callback = [](const vortex::Graphics& gfx, NodeDesc* initializers) -> std::unique_ptr<INode> {
            auto node = std::make_unique<CRTP>(gfx, initializers);
            return node;
        };
        NodeFactory::RegisterNode(name, callback);
    }
};


} // namespace vortex