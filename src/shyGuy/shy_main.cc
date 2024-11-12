
// *** 3rd Party Includes ***
#include <CLI/CLI.hpp>
#include <fmt/format.h>

// *** Project Includes ***
#include "shy_guy.hpp"
#include "define_arguements.hpp"


auto main(int argc, const char** argv) -> int {

    CLI::App app{ fmt::format("{} version {}", "shyguy dag daemon", "0.0.1") };
    cosmos::shy_arguments arguments {};

    cosmos::define_shyguys_arguments(app, arguments);

    try {
        CLI11_PARSE(app, argc, argv);
        cosmos::shy_guy server(arguments);
        server.run();
    }
    catch (std::exception& e) {
        fmt::println("{}", e.what());
        return EXIT_FAILURE;
    }
}
