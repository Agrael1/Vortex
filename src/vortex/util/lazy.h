#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible destroy function pointer type
typedef void (*LazyDestroyFunc)(void* instance);

// C interface functions
void lazy_register_destroy(void* instance, LazyDestroyFunc destroy_func);

#ifdef __cplusplus
}

#include <vector>
#include <utility>
#include <concepts>

namespace vortex {

// Global registry for lazy destroy functions
class LazyRegistry {
private:
    static std::vector<std::pair<void*, LazyDestroyFunc>>& GetRegistry() {
        static std::vector<std::pair<void*, LazyDestroyFunc>> registry;
        return registry;
    }

public:
    static void Register(void* instance, LazyDestroyFunc destroy_func) {
        GetRegistry().emplace_back(instance, destroy_func);
    }

    static void DestroyAll() {
        auto& registry = GetRegistry();
        for (auto& [instance, destroy_func] : registry) {
            if (destroy_func) {
                destroy_func(instance);
            }
        }
        registry.clear();
    }
};


template<typename CRTP>
struct Lazy
{
public:
    // Register on construction
    Lazy()
    {
        auto destroy_func = [](void* ptr) {
            static_cast<CRTP*>(ptr)->Destroy();
        };
        LazyRegistry::Register(this, destroy_func);
    }
};

// Determines where all the lazy structures are destroyed
struct LazyToken {
    ~LazyToken() noexcept
    {
        LazyRegistry::DestroyAll();
    }
};

}

// C interface implementations
extern "C" inline void lazy_register_destroy(void* instance, LazyDestroyFunc destroy_func)
{
    vortex::LazyRegistry::Register(instance, destroy_func);
}


#endif