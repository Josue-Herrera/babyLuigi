//
// Created by Josue Herrera on 11/10/24.
//
#pragma once
#ifndef ARGUMENT_PARSING_HPP
#define ARGUMENT_PARSING_HPP

// *** 3rd Party Includes ***
#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include "shyguy_request.hpp"


namespace cosmos::inline v1
{

    struct shy_guy_info
    {
        std::string port {"90909"};
        std::string server_address{"127.0.0.1"};
    };

    struct shy_arguments {
        std::string port{ "90909" };
        unsigned max_task_graphs{ 1000 };
        unsigned max_dag_concurrency{ 2 };
        unsigned max_task_concurrency{ 4 };
        unsigned execution_idle_ms{ 500 };
        bool interactive {true};
    };

    struct baby_cli_state {
        cosmos::shy_guy_info info{};
        cosmos::shyguy_request request{};
    };

    inline auto define_shyguys_arguments (CLI::App& app, shy_arguments& defaults) {
        app.add_option("-p,--port", defaults.port,
            fmt::format("Port to listen to baby luigi (default: {})", defaults.port));

        app.add_option("-m,--max-graphs", defaults.max_task_graphs,
            fmt::format("Max number of task graphs (default: {})", defaults.max_task_graphs));

        app.add_option("--max-dag-concurrency", defaults.max_dag_concurrency,
            fmt::format("Max concurrent DAG runs (default: {})", defaults.max_dag_concurrency));

        app.add_option("--max-task-concurrency", defaults.max_task_concurrency,
            fmt::format("Max concurrent tasks per DAG (default: {})", defaults.max_task_concurrency));

        app.add_option("--execution-idle-ms", defaults.execution_idle_ms,
            fmt::format("Executioner idle shutdown milliseconds (default: {})", defaults.execution_idle_ms));

        app.add_flag("-i,--interactive", defaults.interactive,
            fmt::format("Use interactive TUI (default: {})", defaults.max_task_graphs));
    }

    inline auto add_babyluigi_subcommands(CLI::App& app, baby_cli_state& state) -> void {
        // Connection options (shared)
        app.add_option("--port", state.info.port,
                       fmt::format("Port to connect to ShyGuy (default: {})", state.info.port));
        app.add_option("--server", state.info.server_address,
                       fmt::format("ShyGuy address (default: {})", state.info.server_address));

        // DAG commands
        auto* dag = app.add_subcommand("dag", "Operate on DAGs");
        dag->require_subcommand(1);

        // dag create
        auto* dag_create = dag->add_subcommand("create", "Create a new DAG");
        std::string dag_name_create;
        std::string dag_schedule;
        dag_create->add_option("-n,--name", dag_name_create, "DAG name")->required();
        dag_create->add_option("-s,--schedule", dag_schedule, "Optional cron schedule string");
        dag_create->callback([&]() {
            cosmos::shyguy_dag dag_req{};
            dag_req.name = dag_name_create;
            if (!dag_schedule.empty()) dag_req.schedule = dag_schedule;
            state.request.data = dag_req;
            state.request.command = cosmos::command_enum::create;
        });

        // dag remove
        auto* dag_remove = dag->add_subcommand("remove", "Remove a DAG");
        std::string dag_name_remove;
        dag_remove->add_option("-n,--name", dag_name_remove, "DAG name")->required();
        dag_remove->callback([&]() {
            cosmos::shyguy_dag dag_req{};
            dag_req.name = dag_name_remove;
            state.request.data = dag_req;
            state.request.command = cosmos::command_enum::remove;
        });

        // dag execute
        auto* dag_execute = dag->add_subcommand("execute", "Execute a DAG now");
        std::string dag_name_execute;
        dag_execute->add_option("-n,--name", dag_name_execute, "DAG name")->required();
        dag_execute->callback([&]() {
            cosmos::shyguy_dag dag_req{};
            dag_req.name = dag_name_execute;
            state.request.data = dag_req;
            state.request.command = cosmos::command_enum::execute;
        });

        // dag snapshot
        auto* dag_snapshot = dag->add_subcommand("snapshot", "Snapshot DAG state");
        std::string dag_name_snapshot;
        dag_snapshot->add_option("-n,--name", dag_name_snapshot, "DAG name")->required();
        dag_snapshot->callback([&]() {
            cosmos::shyguy_dag dag_req{};
            dag_req.name = dag_name_snapshot;
            state.request.data = dag_req;
            state.request.command = cosmos::command_enum::snapshot;
        });

        // TASK commands
        auto* task = app.add_subcommand("task", "Operate on Tasks");
        task->require_subcommand(1);

        // task create
        auto* task_create = task->add_subcommand("create", "Create a new Task");
        std::string task_name_create;
        std::string task_dag_create;
        std::string task_file_location;
        std::vector<std::string> dep_names;
        bool python_type = false;
        bool shell_type = false;
        task_create->add_option("-n,--name", task_name_create, "Task name")->required();
        task_create->add_option("-g,--dag", task_dag_create, "Associated DAG name")->required();
        task_create->add_option("-t,--task", task_file_location, "Path to task file")->required();
        task_create->add_option("-d,--dependencies", dep_names, "Dependent task names");
        auto* type_group = task_create->add_option_group("type", "Task type");
        type_group->add_flag("-p,--python", python_type, "Python task type")->ignore_case();
        type_group->add_flag("-s,--shell", shell_type, "Shell script task type")->ignore_case();
        type_group->require_option(1);
        task_create->callback([&]() {
            cosmos::shyguy_task t{};
            t.name = task_name_create;
            t.associated_dag = task_dag_create;
            t.filename = task_file_location;
            if (!dep_names.empty()) t.dependency_names = dep_names;
            if (python_type) t.type = std::string{"python"};
            if (shell_type) t.type = std::string{"shell"};
            state.request.data = t;
            state.request.command = cosmos::command_enum::create;
        });

        // task remove
        auto* task_remove = task->add_subcommand("remove", "Remove a Task");
        std::string task_name_remove;
        std::string task_dag_remove;
        task_remove->add_option("-n,--name", task_name_remove, "Task name")->required();
        task_remove->add_option("-g,--dag", task_dag_remove, "Associated DAG name")->required();
        task_remove->callback([&]() {
            cosmos::shyguy_task t{};
            t.name = task_name_remove;
            t.associated_dag = task_dag_remove;
            state.request.data = t;
            state.request.command = cosmos::command_enum::remove;
        });

        // task execute
        auto* task_execute = task->add_subcommand("execute", "Execute a Task");
        std::string task_name_execute;
        std::string task_dag_execute;
        task_execute->add_option("-n,--name", task_name_execute, "Task name")->required();
        task_execute->add_option("-g,--dag", task_dag_execute, "Associated DAG name")->required();
        task_execute->callback([&]() {
            cosmos::shyguy_task t{};
            t.name = task_name_execute;
            t.associated_dag = task_dag_execute;
            state.request.data = t;
            state.request.command = cosmos::command_enum::execute;
        });

        // task snapshot
        auto* task_snapshot = task->add_subcommand("snapshot", "Snapshot a Task");
        std::string task_name_snapshot;
        std::string task_dag_snapshot;
        task_snapshot->add_option("-n,--name", task_name_snapshot, "Task name")->required();
        task_snapshot->add_option("-g,--dag", task_dag_snapshot, "Associated DAG name")->required();
        task_snapshot->callback([&]() {
            cosmos::shyguy_task t{};
            t.name = task_name_snapshot;
            t.associated_dag = task_dag_snapshot;
            state.request.data = t;
            state.request.command = cosmos::command_enum::snapshot;
        });
    }

} // namespace cosmos::inline v1

#endif //ARGUMENT_PARSING_HPP
