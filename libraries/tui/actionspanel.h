//
// Created by jojo on 12/15/2025.
//
#pragma once
#ifndef ACTIONSPANEL_H
#define ACTIONSPANEL_H

#include <ftxui/component/component.hpp>

#include "fwd_vocabulary.hpp"

namespace cosmos::inline v1
{
    class actions_panel
    {
    public:
        actions_panel(concurrent_shyguy_t shyguy, input_queue_t io_queue);
        [[nodiscard]] auto render() const -> ftxui::Component { return renderer; }

    private:
        void rebuild();
        void refresh_lists();

        concurrent_shyguy_t shyguy;
        input_queue_t io_queue;

        std::int32_t selected_target{}; // 0 = dag, 1 = task
        std::int32_t selected_action{};
        std::int32_t selected_dag{};
        std::int32_t selected_task{};

        std::vector<std::string> target_names{"dag", "task"};
        std::vector<std::string> action_names{
            "create",
            "delete",
            "update",
            "execute",
            "restart",
            "snapshot",
        };

        std::vector<std::string> dag_names;
        std::vector<std::pair<std::string, std::string>> task_refs; // (dag, task)
        std::vector<std::string> task_labels;

        std::string status_line{"Enter: send action | r: refresh | u: use selected"};

        std::string dag_name_input;
        std::int32_t dag_schedule_kind{}; // 0 none, 1 hourly ... 6 custom
        std::vector<std::string> dag_schedule_kind_names{"none", "hourly", "daily", "weekly", "monthly", "yearly", "custom"};

        std::string cron_custom_input;
        std::string cron_minute_input{"0"};
        std::string cron_hour_input{"0"};
        std::string cron_day_of_month_input{"1"};
        std::string cron_month_input{"1"};
        std::string cron_day_of_week_input{"0"}; // 0=Sun..6=Sat

        std::string task_name_input;
        std::string task_dag_input;
        std::int32_t task_type_index{}; // 0 python, 1 shell
        std::vector<std::string> task_type_names{"python", "shell"};
        std::string task_file_path_input;
        std::string task_dependencies_input; // comma separated

        ftxui::Component target_radio;
        ftxui::Component action_menu;
        ftxui::Component dag_menu;
        ftxui::Component task_menu;

        ftxui::Component dag_name_field;
        ftxui::Component dag_schedule_kind_radio;
        ftxui::Component cron_custom_field;
        ftxui::Component cron_minute_field;
        ftxui::Component cron_hour_field;
        ftxui::Component cron_day_of_month_field;
        ftxui::Component cron_month_field;
        ftxui::Component cron_day_of_week_field;
        ftxui::Component task_name_field;
        ftxui::Component task_dag_field;
        ftxui::Component task_type_radio;
        ftxui::Component task_file_field;
        ftxui::Component task_deps_field;

        ftxui::Component submit_button;

        ftxui::Component container;
        ftxui::Component renderer;
    };
}

#endif // ACTIONSPANEL_H
