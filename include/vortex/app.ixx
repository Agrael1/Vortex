export module vortex.app;

import vortex.graphics;

namespace vortex {
    export class App {
    public:
        App()
            : gfx(nullptr, true)
        {
        }
    private:
        vortex::Graphics gfx;
    };
}