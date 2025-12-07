#pragma once
#include <concepts>
#include <vortex/ui/value.h>
#include <string>

namespace vortex::ui {
template<typename F>
concept MemberFunction = std::is_member_function_pointer_v<F>;

template<typename Func>
struct MemFuncExtractor;

template<typename R, typename Class, typename... Args>
struct MemFuncExtractor<R (Class::*)(Args...)> {
    using return_type = R;
    using class_type = Class;
    using param_types = std::tuple<Args...>;
    static constexpr size_t param_count = sizeof...(Args);

    static auto extract_arguments(CefListValue& list, bool& success)
    {
        return vortex::ui::extract_arguments<Args...>(list, success);
    }
};
template<typename R, typename Class, typename... Args>
struct MemFuncExtractor<R (Class::*)(Args...) const> {
    using return_type = R;
    using class_type = Class;
    using param_types = std::tuple<Args...>;
    static constexpr size_t param_count = sizeof...(Args);

    static auto extract_arguments(CefListValue& list, bool& success)
    {
        return vortex::ui::extract_arguments<Args...>(list, success);
    }
};

template<auto MemberFunc>
struct MessageDispatch {
    static_assert(MemberFunction<decltype(MemberFunc)>, "MemberFunc must be a member function pointer");
    using Extractor = MemFuncExtractor<decltype(MemberFunc)>;
    using return_type = typename Extractor::return_type;
    using class_type = typename Extractor::class_type;
    using param_types = typename Extractor::param_types;
    static constexpr size_t param_count = Extractor::param_count;

public:
    static void Dispatch(class_type& instance, CefListValue& args)
    {
        bool success = false;
        if (args.GetSize() != param_count) {
            vortex::warn("Argument count mismatch: expected {}, got {} (types: {})",
                         param_count,
                         args.GetSize(),
                         DescribeArgumentTypes(args));
            ResolveFailure(instance);
            return;
        }

        if constexpr (param_count == 0) {
            if constexpr (std::is_void_v<return_type>) {
                (instance.*MemberFunc)();
                return;
            } else {
                instance.HandleUIReturn((instance.*MemberFunc)());
                return;
            }
        } else {
            auto tuple = Extractor::extract_arguments(args, success);
            if (!success) {
                vortex::warn("Failed to extract arguments for message dispatch (types: {})",
                             DescribeArgumentTypes(args));
                ResolveFailure(instance);
                return;
            }
            if constexpr (std::is_void_v<return_type>) {
                std::apply([&](auto&&... unpacked_args) {
                    (instance.*MemberFunc)(std::forward<decltype(unpacked_args)>(unpacked_args)...);
                },
                           tuple);
            } else {
                instance.HandleUIReturn(std::apply([&](auto&&... unpacked_args) {
                    return (instance.*MemberFunc)(std::forward<decltype(unpacked_args)>(unpacked_args)...);
                },
                                         tuple));
            }
        }
    }

private:
    static std::string DescribeArgumentTypes(CefListValue& args)
    {
        std::string buffer;
        for (size_t i = 0; i < args.GetSize(); ++i) {
            if (i > 0) {
                buffer.append(", ");
            }
            buffer.append(reflect::enum_name(args.GetType(i)));
        }
        return buffer;
    }

    static void ResolveFailure(class_type& instance)
    {
        if constexpr (std::is_same_v<return_type, bool>) {
            instance.HandleUIReturn(false);
        }
    }
};
} // namespace vortex::ui