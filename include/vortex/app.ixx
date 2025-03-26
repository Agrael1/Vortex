#ifndef __INTELLISENSE__
export module vortex.app;
import wisdom;
#else
#include <wisdom/wisdom.hpp>
#endif

namespace vortex {
    export class App {
    public:
        App() {
        }
    private:
        wis::Factory factory;
    };
}