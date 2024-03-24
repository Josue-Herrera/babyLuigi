#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <shy_guy.hpp>

auto main(int argc, const char** argv) -> int {
    CLI::App app{ fmt::format("{} version {}",
                             "shy guy is baby luigi's task daemon", "0.0.0") };

    unsigned port{ 90909 }, max_task_graphs{ 1000 };
    app.add_option(
        "-p,--port", port,
        fmt::format("Port to listen to baby luigi (default: {})", port));
    app.add_option(
        "-m,--max-graphs", max_task_graphs,
        fmt::format("Max number of task graphs (default: {})", max_task_graphs));
    CLI11_PARSE(app, argc, argv);

    jx::shy_guy server(port);

    test_json("{ \"payload\" : [100,101,102] }");

  //server.run();

  // shyGuy flow

  // [x] todo: validate command line args

  // async (asio)
  // [ ] todo: wait for connections from babyLuigi

  // [ ] todo: wait for request

  // [ ] todo: try and service the request (many sub tasks here)

  // [ ] todo: send response to request

  return 0;
}
