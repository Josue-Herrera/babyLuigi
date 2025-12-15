//
// Created by jojo on 12/15/2025.
//

#include "actionspanel.h"

#include <blocking_queue.hpp>
#include <threadsafe_shyguy/concurrent_shyguy.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <charconv>
#include <fstream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cosmos::inline v1
{
    namespace
    {
        [[nodiscard]] auto command_for_action(std::string_view action) -> std::optional<command_enum>
        {
            if (action == "create")
                return command_enum::create;
            if (action == "delete")
                return command_enum::remove;
            if (action == "update")
                return command_enum::create;
            if (action == "execute")
                return command_enum::execute;
            if (action == "restart")
                return command_enum::execute;
            if (action == "snapshot")
                return command_enum::snapshot;

            return std::nullopt;
        }

        [[nodiscard]] auto clamp_index(std::int32_t value, std::size_t size) -> std::int32_t
        {
            if (size == 0U)
                return 0;
            return std::clamp(value, std::int32_t{0}, static_cast<std::int32_t>(size - 1U));
        }

        [[nodiscard]] auto trim(std::string_view input) -> std::string
        {
            auto begin = input.begin();
            auto end = input.end();
            while (begin != end && std::isspace(static_cast<unsigned char>(*begin)))
                ++begin;
            while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1))))
                --end;
            return {begin, end};
        }

        [[nodiscard]] auto split_csv(std::string_view input) -> std::vector<std::string>
        {
            std::vector<std::string> out;
            std::string current;
            for (char ch : input)
            {
                if (ch == ',')
                {
                    auto token = trim(current);
                    if (not token.empty())
                        out.push_back(std::move(token));
                    current.clear();
                    continue;
                }
                current.push_back(ch);
            }

            auto token = trim(current);
            if (not token.empty())
                out.push_back(std::move(token));
            return out;
        }

        [[nodiscard]] auto read_file(std::string const& path) -> std::optional<std::string>
        {
            std::ifstream input{path, std::ios::binary};
            if (not input.is_open())
                return std::nullopt;
            std::ostringstream buffer;
            buffer << input.rdbuf();
            return buffer.str();
        }

        [[nodiscard]] auto parse_int_in_range(std::string_view input, int min_value, int max_value) -> std::optional<int>
        {
            auto s = trim(input);
            if (s.empty())
                return std::nullopt;

            int value{};
            auto const* begin = s.data();
            auto const* end = s.data() + s.size();
            auto [ptr, ec] = std::from_chars(begin, end, value);
            if (ec != std::errc{} || ptr != end)
                return std::nullopt;

            if (value < min_value || value > max_value)
                return std::nullopt;
            return value;
        }

        [[nodiscard]] auto build_cron_from_form(
            int schedule_kind,
            std::string_view cron_custom,
            std::string_view minute,
            std::string_view hour,
            std::string_view day_of_month,
            std::string_view month,
            std::string_view day_of_week) -> std::optional<std::string>
        {
            // Cron format used by this repo: "min hour dom month dow" (5 fields).
            switch (schedule_kind)
            {
                case 0: // none
                    return std::nullopt;
                case 6: // custom
                {
                    auto cron = trim(cron_custom);
                    if (cron.empty())
                        return std::nullopt;
                    return cron;
                }
                case 1: // hourly
                {
                    auto m = parse_int_in_range(minute, 0, 59);
                    if (not m)
                        return std::nullopt;
                    return std::to_string(*m) + " * * * *";
                }
                case 2: // daily
                {
                    auto m = parse_int_in_range(minute, 0, 59);
                    auto h = parse_int_in_range(hour, 0, 23);
                    if (not m || not h)
                        return std::nullopt;
                    return std::to_string(*m) + " " + std::to_string(*h) + " * * *";
                }
                case 3: // weekly
                {
                    auto m = parse_int_in_range(minute, 0, 59);
                    auto h = parse_int_in_range(hour, 0, 23);
                    auto dow = parse_int_in_range(day_of_week, 0, 6);
                    if (not m || not h || not dow)
                        return std::nullopt;
                    return std::to_string(*m) + " " + std::to_string(*h) + " * * " + std::to_string(*dow);
                }
                case 4: // monthly
                {
                    auto m = parse_int_in_range(minute, 0, 59);
                    auto h = parse_int_in_range(hour, 0, 23);
                    auto dom = parse_int_in_range(day_of_month, 1, 31);
                    if (not m || not h || not dom)
                        return std::nullopt;
                    return std::to_string(*m) + " " + std::to_string(*h) + " " + std::to_string(*dom) + " * *";
                }
                case 5: // yearly
                {
                    auto m = parse_int_in_range(minute, 0, 59);
                    auto h = parse_int_in_range(hour, 0, 23);
                    auto dom = parse_int_in_range(day_of_month, 1, 31);
                    auto mon = parse_int_in_range(month, 1, 12);
                    if (not m || not h || not dom || not mon)
                        return std::nullopt;
                    return std::to_string(*m) + " " + std::to_string(*h) + " " + std::to_string(*dom) + " " + std::to_string(*mon) + " *";
                }
                default:
                    return std::nullopt;
            }
        }
    } // namespace

    actions_panel::actions_panel(concurrent_shyguy_t shyguy_in, input_queue_t io_queue_in)
        : shyguy{std::move(shyguy_in)}
        , io_queue{std::move(io_queue_in)}
    {
        refresh_lists();
        rebuild();
    }

    void actions_panel::refresh_lists()
    {
        dag_names.clear();
        task_refs.clear();
        task_labels.clear();

        if (not shyguy)
        {
            status_line = "Shyguy unavailable";
            return;
        }

        auto storage = shyguy->storage_handle();
        if (not storage)
        {
            status_line = "Storage unavailable";
            return;
        }

        auto dags_expected = storage->dags().list_dags();
        if (not dags_expected)
        {
            status_line = "Unable to list DAGs";
            return;
        }

        for (auto const& dag_entry : dags_expected.value())
        {
            auto const& dag_name = dag_entry.value.name;
            dag_names.push_back(dag_name);
            auto tasks_expected = storage->tasks().list_tasks(dag_name);
            if (not tasks_expected)
                continue;

            for (auto const& task_entry : tasks_expected.value())
            {
                task_refs.emplace_back(dag_name, task_entry.value.name);
            }
        }

        task_labels = task_refs
            | std::views::transform([](auto const& pair)
            {
                return pair.first + " :: " + pair.second;
            })
            | std::ranges::to<std::vector>();

        if (dag_names.empty())
            status_line = "No DAGs available.";
        else if (task_labels.empty())
            status_line = "No tasks available (create tasks first).";

        selected_target = clamp_index(selected_target, target_names.size());
        selected_action = clamp_index(selected_action, action_names.size());
        selected_dag = clamp_index(selected_dag, dag_names.size());
        selected_task = clamp_index(selected_task, task_labels.size());
    }

    void actions_panel::rebuild()
    {
        target_radio = ftxui::Radiobox(&target_names, &selected_target);
        action_menu = ftxui::Menu(&action_names, &selected_action);
        dag_menu = ftxui::Menu(&dag_names, &selected_dag);
        task_menu = ftxui::Menu(&task_labels, &selected_task);

        dag_name_field = ftxui::Input(&dag_name_input, "dag name");
        dag_schedule_kind_radio = ftxui::Radiobox(&dag_schedule_kind_names, &dag_schedule_kind);
        cron_custom_field = ftxui::Input(&cron_custom_input, "cron: m h dom mon dow");
        cron_minute_field = ftxui::Input(&cron_minute_input, "minute (0-59)");
        cron_hour_field = ftxui::Input(&cron_hour_input, "hour (0-23)");
        cron_day_of_month_field = ftxui::Input(&cron_day_of_month_input, "day of month (1-31)");
        cron_month_field = ftxui::Input(&cron_month_input, "month (1-12)");
        cron_day_of_week_field = ftxui::Input(&cron_day_of_week_input, "day of week (0=Sun..6=Sat)");

        task_name_field = ftxui::Input(&task_name_input, "task name");
        task_dag_field = ftxui::Input(&task_dag_input, "associated dag");
        task_type_radio = ftxui::Radiobox(&task_type_names, &task_type_index);
        task_file_field = ftxui::Input(&task_file_path_input, "task file path");
        task_deps_field = ftxui::Input(&task_dependencies_input, "deps: a,b,c");

        auto submit_request = [&]() -> bool
        {
            if (not io_queue)
            {
                status_line = "No IO queue available";
                return true;
            }

            auto const safe_action_index = clamp_index(selected_action, action_names.size());
            auto const action = action_names.at(static_cast<std::size_t>(safe_action_index));
            auto command = command_for_action(action);
            if (not command)
            {
                status_line = "Action '" + action + "' is not supported yet.";
                return true;
            }

            shyguy_request request{};
            request.command = *command;

            auto const is_dag = (selected_target == 0);
            auto const is_create_or_update = (action == "create" || action == "update");

            if (is_dag)
            {
                std::string dag_name{};
                std::optional<std::string> schedule{};

                if (is_create_or_update)
                {
                    dag_name = trim(dag_name_input);
                    auto cron = build_cron_from_form(
                        dag_schedule_kind,
                        cron_custom_input,
                        cron_minute_input,
                        cron_hour_input,
                        cron_day_of_month_input,
                        cron_month_input,
                        cron_day_of_week_input);
                    if (dag_schedule_kind != 0 && not cron)
                    {
                        status_line = "Invalid schedule input";
                        return true;
                    }
                    if (cron)
                        schedule = *cron;
                }
                else
                {
                    if (dag_names.empty())
                    {
                        status_line = "No DAG selected";
                        return true;
                    }
                    dag_name = dag_names.at(static_cast<std::size_t>(clamp_index(selected_dag, dag_names.size())));
                }

                if (dag_name.empty())
                {
                    status_line = "DAG name is required";
                    return true;
                }

                request.emplace(shyguy_dag{.name = dag_name, .schedule = schedule});
                io_queue->enqueue(std::move(request));
                status_line = "Queued '" + action + "' for dag '" + dag_name + "'";
                return true;
            }

            // task
            if (is_create_or_update)
            {
                auto task_name = trim(task_name_input);
                auto dag_name = trim(task_dag_input);
                auto file_path = trim(task_file_path_input);

                if (task_name.empty())
                {
                    status_line = "Task name is required";
                    return true;
                }
                if (dag_name.empty())
                {
                    status_line = "Associated DAG is required";
                    return true;
                }
                if (file_path.empty())
                {
                    status_line = "Task file path is required";
                    return true;
                }

                auto contents = read_file(file_path);
                if (not contents)
                {
                    status_line = "Unable to read task file: " + file_path;
                    return true;
                }

                auto deps = split_csv(task_dependencies_input);
                std::optional<std::vector<std::string>> dep_opt{};
                if (not deps.empty())
                    dep_opt = std::move(deps);

                auto type = task_type_names.at(static_cast<std::size_t>(clamp_index(task_type_index, task_type_names.size())));

                request.emplace(shyguy_task{
                    .name = task_name,
                    .associated_dag = dag_name,
                    .type = std::string{type},
                    .filename = file_path,
                    .file_content = std::move(*contents),
                    .dependency_names = std::move(dep_opt),
                });

                io_queue->enqueue(std::move(request));
                status_line = "Queued '" + action + "' for task '" + dag_name + " :: " + task_name + "'";
                return true;
            }

            if (task_refs.empty())
            {
                status_line = "No task selected";
                return true;
            }

            auto const safe_task_index = clamp_index(selected_task, task_refs.size());
            auto const& [dag_name, task_name] = task_refs.at(static_cast<std::size_t>(safe_task_index));
            request.emplace(shyguy_task{
                .name = task_name,
                .associated_dag = dag_name,
            });

            io_queue->enqueue(std::move(request));
            status_line = "Queued '" + action + "' for " + dag_name + " :: " + task_name;
            return true;
        };

        submit_button = ftxui::Button("submit", submit_request);

        auto form_tab = ftxui::Container::Tab({
            ftxui::Container::Vertical({
                dag_name_field,
                dag_schedule_kind_radio,
                cron_custom_field,
                cron_minute_field,
                cron_hour_field,
                cron_day_of_month_field,
                cron_month_field,
                cron_day_of_week_field,
            }),
            ftxui::Container::Vertical({task_name_field, task_dag_field, task_type_radio, task_file_field, task_deps_field}),
        }, &selected_target);

        auto left_container = ftxui::Container::Vertical({target_radio, action_menu});
        auto right_container = ftxui::Container::Vertical({form_tab, submit_button});

        container = ftxui::Container::Horizontal({
            left_container,
            right_container,
        });

        renderer = ftxui::CatchEvent(ftxui::Renderer(container, [&]
        {
            auto const is_dag = (selected_target == 0);
            auto const safe_action_index = clamp_index(selected_action, action_names.size());
            auto const action = action_names.empty() ? std::string{} : action_names.at(static_cast<std::size_t>(safe_action_index));
            auto const is_create_or_update = (action == "create" || action == "update");

            auto left = ftxui::vbox({
                ftxui::text("target") | ftxui::bold,
                target_radio->Render(),
                ftxui::separator(),
                ftxui::text("action") | ftxui::bold,
                action_menu->Render() | ftxui::flex,
            }) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 18);

            ftxui::Elements form_lines;
            form_lines.push_back(ftxui::text("form") | ftxui::bold);

            if (is_dag)
            {
                if (is_create_or_update)
                {
                    form_lines.push_back(ftxui::text("dag name:"));
                    form_lines.push_back(dag_name_field->Render());
                    form_lines.push_back(ftxui::separator());
                    form_lines.push_back(ftxui::text("schedule builder:") | ftxui::bold);
                    form_lines.push_back(dag_schedule_kind_radio->Render());
                    form_lines.push_back(ftxui::separator());

                    auto cron_preview = build_cron_from_form(
                        dag_schedule_kind,
                        cron_custom_input,
                        cron_minute_input,
                        cron_hour_input,
                        cron_day_of_month_input,
                        cron_month_input,
                        cron_day_of_week_input);
                    form_lines.push_back(ftxui::text("cron preview: " + (cron_preview ? *cron_preview : std::string{"<none>"})) | ftxui::dim);

                    if (dag_schedule_kind == 6)
                    {
                        form_lines.push_back(ftxui::text("custom cron:"));
                        form_lines.push_back(cron_custom_field->Render());
                    }
                    else if (dag_schedule_kind == 1)
                    {
                        form_lines.push_back(ftxui::text("minute:"));
                        form_lines.push_back(cron_minute_field->Render());
                    }
                    else if (dag_schedule_kind == 2)
                    {
                        form_lines.push_back(ftxui::text("hour + minute:"));
                        form_lines.push_back(cron_hour_field->Render());
                        form_lines.push_back(cron_minute_field->Render());
                    }
                    else if (dag_schedule_kind == 3)
                    {
                        form_lines.push_back(ftxui::text("weekday + time:"));
                        form_lines.push_back(cron_day_of_week_field->Render());
                        form_lines.push_back(cron_hour_field->Render());
                        form_lines.push_back(cron_minute_field->Render());
                    }
                    else if (dag_schedule_kind == 4)
                    {
                        form_lines.push_back(ftxui::text("day-of-month + time:"));
                        form_lines.push_back(cron_day_of_month_field->Render());
                        form_lines.push_back(cron_hour_field->Render());
                        form_lines.push_back(cron_minute_field->Render());
                    }
                    else if (dag_schedule_kind == 5)
                    {
                        form_lines.push_back(ftxui::text("month + day + time:"));
                        form_lines.push_back(cron_month_field->Render());
                        form_lines.push_back(cron_day_of_month_field->Render());
                        form_lines.push_back(cron_hour_field->Render());
                        form_lines.push_back(cron_minute_field->Render());
                    }
                }
                else
                {
                    auto selected_name = dag_names.empty() ? std::string{"<none>"} : dag_names.at(static_cast<std::size_t>(clamp_index(selected_dag, dag_names.size())));
                    form_lines.push_back(ftxui::text("selected dag: " + selected_name));
                    form_lines.push_back(dag_menu->Render() | ftxui::flex);
                }
            }
            else
            {
                if (is_create_or_update)
                {
                    form_lines.push_back(ftxui::text("task name:"));
                    form_lines.push_back(task_name_field->Render());
                    form_lines.push_back(ftxui::text("dag:"));
                    form_lines.push_back(task_dag_field->Render());
                    form_lines.push_back(ftxui::text("type:"));
                    form_lines.push_back(task_type_radio->Render());
                    form_lines.push_back(ftxui::text("task file:"));
                    form_lines.push_back(task_file_field->Render());
                    form_lines.push_back(ftxui::text("dependencies (csv):"));
                    form_lines.push_back(task_deps_field->Render());
                }
                else
                {
                    auto selected_label = task_labels.empty() ? std::string{"<none>"} : task_labels.at(static_cast<std::size_t>(clamp_index(selected_task, task_labels.size())));
                    form_lines.push_back(ftxui::text("selected task: " + selected_label));
                    form_lines.push_back(task_menu->Render() | ftxui::flex);
                }
            }

            auto form = ftxui::vbox(std::move(form_lines)) | ftxui::flex;

            auto footer = ftxui::hbox({
                ftxui::text(status_line) | ftxui::dim | ftxui::flex,
                submit_button->Render(),
            });

            return ftxui::hbox({
                left,
                ftxui::separator(),
                ftxui::vbox({form, ftxui::separator(), footer}) | ftxui::flex,
            }) | ftxui::flex;
        }),
        [&](ftxui::Event const& event)
        {
            if (event == ftxui::Event::Character('r') || event == ftxui::Event::Character('R'))
            {
                refresh_lists();
                rebuild();
                return true;
            }

            if (event == ftxui::Event::Character('u') || event == ftxui::Event::Character('U'))
            {
                if (selected_target == 0 && not dag_names.empty())
                {
                    auto const name = dag_names.at(static_cast<std::size_t>(clamp_index(selected_dag, dag_names.size())));
                    dag_name_input = name;
                }
                else if (selected_target == 1 && not task_refs.empty())
                {
                    auto const& [dag, task] = task_refs.at(static_cast<std::size_t>(clamp_index(selected_task, task_refs.size())));
                    task_dag_input = dag;
                    task_name_input = task;
                }
                return true;
            }

            if (event != ftxui::Event::Return)
                return false;

            return submit_request();
        });
    }
}
