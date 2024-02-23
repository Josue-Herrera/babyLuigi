
#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <string>
#include <vector>

auto main(int argc, const char **argv) -> int
{  
  CLI::App app{fmt::format("{} version {}",
                           "shy guy is baby luigi's task daemon", "0.0.0")};

  // API general babyluigi -[p|s] python or script -[n|--name] <name> -[t|task] <rel location of file> -[d|--dependencies] <list of dependant names> 

  bool python_type, script_type {};
  auto task_type = app.add_option_group("task_subgroup");
  task_type->add_flag("-p,--python", python_type, "Python task type")->ignore_case();
  task_type->add_flag("-s,--script", script_type, "Script task type")->ignore_case();
  task_type->required(true);

  std::string task_name{};
  app.add_option(
      "-n,--name", task_name,
      "Task name");

  std::string task_file_location{};
  app.add_option(
      "-t,--task", task_file_location,
      "Relative location of task location");

  std::vector<std::string> dependency_names{};
  app.add_option(
      "-d,--dependencies", dependency_names,
      "List of dependent names for the task");

  unsigned port {9999};
  app.add_option("--port", port, 
       fmt::format("Port to listen to shy guy (default: {})", port));
  
  std::string server_address{"127.0.0.1"};
  app.add_option("--server", server_address,
       fmt::format("Address to shy guy (default: {})", server_address));


  CLI11_PARSE(app, argc, argv);

    // BabyLuigi flow

    // validate command line args 

    // connect to shyGuy 

    // build request (many sub tasks here)

    // send request to handle task

    // receive response and inform user

    return 0;
}
