
#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <baby_luigi.hpp>
#include <string>
#include <vector>


auto main(int argc, const char **argv) -> int
{  
  CLI::App app{fmt::format("{}\nversion {}", jx::baby_luigi_banner, "0.0.0")}; 

  // API general babyluigi -[p|s] python or script -[n|--name] <name> -[t|task] <rel location of file> -[d|--dependencies] <list of dependant names> 

  jx::task_description task{};

  bool python_type{}, script_type {};
  auto task_type = app.add_option_group("task_subgroup");
  task_type->add_flag("-p,--python", python_type, "Python task type")->ignore_case();
  task_type->add_flag("-s,--script", script_type, "Script task type")->ignore_case();
  task_type->required(true);

  app.add_option(
      "-n,--name", task.name,
      "Task name")
       ->required();

  app.add_option(
      "-t,--task", task.file_location,
      "Relative location of task location")
       ->required();

  app.add_option(
      "-d,--dependencies", task.dependency_names,
      "List of dependent names for the task");

  jx::shy_guy_info info{};
  app.add_option("--port", info.port, 
       fmt::format("Port to listen to shy guy (default: {})", info.port));
  
  app.add_option("--server", info.server_address,
       fmt::format("Address to shy guy (default: {})", info.server_address));

  CLI11_PARSE(app, argc, argv);

  task.type = (python_type) ? "python" :
              (script_type) ? "script" : "UNKNOWN";

  // BabyLuigi flow
  if (not jx::valid(task)) 
     return EXIT_FAILURE;

  task.payload = jx::download_contents(task.file_location);

  if (not jx::submit_task_request(task,info))
     return EXIT_FAILURE;

    return EXIT_SUCCESS;
}