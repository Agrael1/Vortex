#include <span>
#include <string_view>
#include <memory>
#include <iostream>
#include <source_location>

import vortex.app;


struct MainArgs {
};

int entry_main(std::span<std::string_view> args) try
{
    MainArgs main_args;

    // parse args
    // run main function
    // return result
    vortex::App a{};
    return 0;
}
catch (const std::exception& e)
{
    // Handle exceptions
    return 1;
}
catch (...)
{
    // Handle unknown exceptions
    return 2;
}


int main(int argc, char* argv[]) {
    // Better main function
    auto args = reinterpret_cast<std::string_view*>(alloca(argc * sizeof(std::string_view)));
    for (int i = 0; i < argc; ++i) {
        args[i] = argv[i];
    }

    return entry_main(std::span(args, argc));
}