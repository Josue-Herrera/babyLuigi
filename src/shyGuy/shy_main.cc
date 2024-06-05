#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <shy_guy.hpp>


auto main(int argc, const char** argv) -> int {

    using namespace std::string_literals;

    CLI::App app{ fmt::format("{} version {}",
                             "shy guy is baby luigi's task daemon", "0.0.0") };

    std::string port{ "90909" };
    unsigned max_task_graphs{ 1000 };
    app.add_option(
        "-p,--port", port,
        fmt::format("Port to listen to baby luigi (default: {})", port));
    app.add_option(
        "-m,--max-graphs", max_task_graphs,
        fmt::format("Max number of task graphs (default: {})", max_task_graphs));

    // add optional config path and have a default path (default could be pound define
    CLI11_PARSE(app, argc, argv);

    jx::shy_guy server(port);
    
    try { 
        server.test_run(); 
    }
    catch (...) {
        return EXIT_FAILURE;
    }
}
