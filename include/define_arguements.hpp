//
// Created by Josue Herrera on 11/10/24.
//
#pragma once
#ifndef ARGUMENT_PARSING_HPP
#define ARGUMENT_PARSING_HPP

// *** 3rd Party Includes ***
#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include "task.hpp"


namespace cosmos {

    struct shy_arguments {
        std::string port{ "90909" };
        unsigned max_task_graphs{ 1000 };
        bool interactive {};
    };

    /*
    struct baby_arguments {
        cosmos::task content{};
        std::string port {"90909"};
        std::string server_address{"127.0.0.1"};
        std::string file_location{};

        bool python_type{};
        bool script_type{};
    };*/

    inline auto define_shyguys_arguments (CLI::App& app, shy_arguments& defaults) {
        app.add_option("-p,--port", defaults.port,
            fmt::format("Port to listen to baby luigi (default: {})", defaults.port));

        app.add_option("-m,--max-graphs", defaults.max_task_graphs,
            fmt::format("Max number of task graphs (default: {})", defaults.max_task_graphs));

        app.add_option("-i,--interactive", defaults.interactive,
            fmt::format("Use interactive TUI (default: {})", defaults.max_task_graphs));
    }

    /*
    inline auto define_babyluigis_arguments (CLI::App& app, baby_arguments& defaults) -> void {

        auto task_type = app.add_option_group("task_subgroup");
        task_type->add_flag("-p,--python", defaults.python_type, "Python task type")->ignore_case();
        task_type->add_flag("-s,--script", defaults.script_type, "Script task type")->ignore_case();
        task_type->required(true);

        app.add_option(
            "-n,--name", defaults.content.name,
            "Task name")
             ->required();

        std::string file_location{};
        app.add_option(
            "-t,--task", defaults.file_location,
            "Relative location of task location")
             ->required();

        std::vector<std::string> dependency_names{};
        app.add_option(
            "-d,--dependencies", dependency_names,
            "List of dependent names for the task");


        app.add_option("--port", info.port,
             fmt::format("Port to listen to shy guy (default: {})", info.port));

        app.add_option("--server", info.server_address,
             fmt::format("Address to shy guy (default: {})", info.server_address));
    } */

} // cosmos

#endif //ARGUMENT_PARSING_HPP
