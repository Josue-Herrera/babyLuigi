
// *** Project Includes ***
#include "baby_luigi.hpp"

// *** 3rd Party Includes ***
#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <string>
#include <vector>

auto main(int const argc, const char **argv) -> int
{
  CLI::App app{fmt::format("{}\nversion {}", cosmos::baby_luigi_banner, "0.0.0")};

  // API general babyluigi -[p|s] python or script -[n|--name] <name> -[t|task] <rel location of file> -[d|--dependencies] <list of dependant names>

  cosmos::task t{};

  bool python_type{}, script_type {};
  auto task_type = app.add_option_group("task_subgroup");
  task_type->add_flag("-p,--python", python_type, "Python task type")->ignore_case();
  task_type->add_flag("-s,--script", script_type, "Script task type")->ignore_case();
  task_type->required(true);

  app.add_option(
      "-n,--name", t.name,
      "Task name")
       ->required();

 std::string file_location{};
  app.add_option(
      "-t,--task", file_location,
      "Relative location of task location")
       ->required();

  std::vector<std::string> dependency_names{};
  app.add_option(
      "-d,--dependencies", dependency_names,
      "List of dependent names for the task");

  cosmos::shy_guy_info info{};
  app.add_option("--port", info.port,
       fmt::format("Port to listen to shy guy (default: {})", info.port));

  app.add_option("--server", info.server_address,
       fmt::format("Address to shy guy (default: {})", info.server_address));

  CLI11_PARSE(app, argc, argv);

  t.type = (python_type) ? "python" : (script_type) ? "script" : "UNKNOWN";

  t.filename.emplace(file_location);
  t.dependency_names.emplace(dependency_names);

  // BabyLuigi flow
  if (not cosmos::valid(t))
     return EXIT_FAILURE;

  t.file_content = cosmos::download_contents(t.filename.value());

  if (not cosmos::submit_task_request(t, info))
     return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
