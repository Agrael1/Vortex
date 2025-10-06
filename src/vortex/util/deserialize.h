#pragma once
#include <vortex/properties/type_traits.h>

namespace vortex {
template<PropertyType type>
struct deserialize_executor {
    using value_type = typename type_traits<type>::type;
    static PropertyValue operator()(std::string_view data) noexcept
    {
        PropertyValue value{};
        auto& v = value.emplace<value_type>();
        if (!reflection_traits<value_type>::deserialize(&v, data)) {
            vortex::error("Failed to deserialize property of type {} from data: {}",
                          reflect::type_name<value_type>(),
                          data);
        }
        return value;
    }
};
template<>
struct deserialize_executor<PropertyType::Void> {
    static PropertyValue operator()(std::string_view data) noexcept { return {}; }
};
} // namespace vortex