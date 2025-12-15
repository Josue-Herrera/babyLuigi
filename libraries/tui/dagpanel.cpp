#include "dagpanel.h"

#include "horizontaltabpanel.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <threadsafe_shyguy/concurrent_shyguy.hpp>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cosmos::inline v1
{
    namespace
    {
        constexpr std::size_t run_history_columns = 15;

        [[nodiscard]] auto get_logger()
        {
            auto logger = spdlog::get("shyguy_logger");
            if (not logger)
                logger = spdlog::basic_logger_mt("shyguy_logger", "logs/shyguy.log", true);
            return logger;
        }

        [[nodiscard]] auto status_glyph(dag_run_status status) -> char
        {
            using enum dag_run_status;
            switch (status)
            {
                case success: return '+';
                case failed:  return '-';
                case skipped: return '*';
                case running: return '>';
                case queued:
                    return 'S'; // "stopped" not currently modeled; using "S" for non-terminal.
                case none:
                default:
                    return ' ';
            }
        }

        [[nodiscard]] auto status_color(dag_run_status status) -> ftxui::Color
        {
            using enum dag_run_status;
            switch (status)
            {
                case success: return ftxui::Color::Green;
                case running: return ftxui::Color::Yellow;
                case failed:  return ftxui::Color::RedLight;
                case skipped: return ftxui::Color::GrayLight;
                case queued:  return ftxui::Color::BlueLight;
                case none:
                default:
                    return ftxui::Color::GrayDark;
            }
        }

        [[nodiscard]] auto frequency_interval(schedule_frequency frequency) -> std::chrono::system_clock::duration
        {
            using namespace std::chrono;
            switch (frequency)
            {
                case schedule_frequency::hourly:  return hours{1};
                case schedule_frequency::daily:   return hours{24};
                case schedule_frequency::weekly:  return hours{24 * 7};
                case schedule_frequency::monthly: return hours{24 * 30};
                case schedule_frequency::yearly:  return hours{24 * 365};
                case schedule_frequency::custom:
                case schedule_frequency::unset:
                case schedule_frequency::unknown:
                default:
                    return hours{24};
            }
        }

        [[nodiscard]] auto format_date_mm_dd_yyyy(std::chrono::system_clock::time_point tp) -> std::string
        {
            auto const time = std::chrono::system_clock::to_time_t(tp);
            std::tm tm{};
#if defined(_WIN32)
            localtime_s(&tm, &time);
#else
            localtime_r(&time, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%m/%d/%Y");
            if (oss.fail())
                return "n/a";
            return oss.str();
        }

        [[nodiscard]] auto infer_frequency(std::span<const task_entry> tasks) -> schedule_frequency
        {
            schedule_frequency inferred = schedule_frequency::unset;
            for (auto const& entry : tasks)
            {
                if (not entry.metadata)
                    continue;

                auto const freq = entry.metadata->schedule.frequency;
                if (freq == schedule_frequency::unset || freq == schedule_frequency::unknown)
                    continue;

                if (inferred == schedule_frequency::unset)
                    inferred = freq;
                else if (inferred != freq)
                    return schedule_frequency::daily;
            }

            if (inferred == schedule_frequency::unset)
                return schedule_frequency::daily;
            return inferred;
        }

        [[nodiscard]] auto make_run_labels(schedule_frequency frequency, std::size_t count) -> std::vector<std::string>
        {
            std::vector<std::string> labels;
            labels.reserve(count);

            auto interval = frequency_interval(frequency);
            if (interval <= std::chrono::system_clock::duration::zero())
                interval = std::chrono::hours{24};

            auto const now = std::chrono::system_clock::now();
            for (std::size_t index = count; index > 0; --index)
            {
                auto const offset = static_cast<int>(index - 1);
                labels.emplace_back(format_date_mm_dd_yyyy(now - interval * offset));
            }
            return labels;
        }

        [[nodiscard]] auto build_dag_task_table(std::string_view dag_name, std::span<const task_entry> tasks) -> ftxui::Element
        {
            if (tasks.empty())
            {
                auto message = std::string{"No tasks registered for "};
                message.append(dag_name);
                return ftxui::text(message) | ftxui::dim;
            }

            auto const frequency = infer_frequency(tasks);
            auto const run_labels = make_run_labels(frequency, run_history_columns);

            std::vector<std::vector<std::string>> table_rows;
            table_rows.reserve(tasks.size() + 1);

            std::vector<std::string> header_row;
            header_row.reserve(run_history_columns + 1);
            header_row.emplace_back("task");
            header_row.insert(header_row.end(), run_labels.begin(), run_labels.end());
            table_rows.push_back(std::move(header_row));

            std::vector<std::vector<dag_run_status>> status_matrix;
            status_matrix.reserve(tasks.size());

            for (auto const& entry : tasks)
            {
                std::vector<std::string> row;
                row.reserve(run_history_columns + 1);
                row.push_back(entry.value.name);

                std::vector<dag_run_status> statuses(run_history_columns, dag_run_status::none);
                if (entry.metadata)
                {
                    statuses.back() = entry.metadata->statuses.current;
                    if (run_history_columns > 1U)
                        statuses[run_history_columns - 2U] = entry.metadata->statuses.previous;
                }

                for (auto const& status : statuses)
                    row.emplace_back(std::string{status_glyph(status)});

                table_rows.push_back(std::move(row));
                status_matrix.push_back(std::move(statuses));
            }

            ftxui::Table table{table_rows};
            table.SelectAll().Border(ftxui::LIGHT);
            table.SelectRow(0).Decorate(ftxui::bold);
            table.SelectRow(0).Decorate(ftxui::bgcolor(ftxui::Color::GrayDark));
            table.SelectRow(0).Decorate(ftxui::color(ftxui::Color::White));

            table.SelectColumn(0).Decorate(ftxui::bold);
            table.SelectColumn(0).Decorate(ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 18));

            for (int col = 1; col <= static_cast<int>(run_history_columns); ++col)
            {
                table.SelectColumn(col).Decorate(ftxui::center);
                table.SelectColumn(col).Decorate(ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 10));
            }

            for (std::size_t row_index = 0; row_index < status_matrix.size(); ++row_index)
            {
                auto const& statuses = status_matrix[row_index];
                for (std::size_t col_index = 0; col_index < statuses.size(); ++col_index)
                {
                    auto selection = table.SelectCell(static_cast<int>(col_index + 1), static_cast<int>(row_index + 1));
                    selection.Decorate(ftxui::bgcolor(status_color(statuses[col_index])));
                    selection.Decorate(ftxui::bold);
                }
            }

            return ftxui::vbox({
                ftxui::text(std::string{dag_name}) | ftxui::bold,
                table.Render(),
            });
        }
    } // namespace

    void dag_panel::update()
    {
        auto const logger = get_logger();
        pages.clear();

        if (not shyguy)
        {
            logger->debug("Shyguy unavailable");
            menu_tab = horizontal_tab_panel(
                {ftxui::Renderer([] { return ftxui::text("Shyguy unavailable") | ftxui::color(ftxui::Color::RedLight); })},
                {"error"});
            return;
        }

        auto storage = shyguy->storage_handle();
        if (not storage)
        {
            logger->debug("Storage unavailable");
            menu_tab = horizontal_tab_panel(
                {ftxui::Renderer([] { return ftxui::text("Storage unavailable") | ftxui::dim; })},
                {"error"});
            return;
        }

        auto dag_entries_expected = storage->dags().list_dags();
        if (not dag_entries_expected)
        {
            logger->debug("Failed to list DAGs");
            menu_tab = horizontal_tab_panel(
                {ftxui::Renderer([] { return ftxui::text("Unable to load DAG metadata") | ftxui::color(ftxui::Color::Orange1); })},
                {"error"});
            return;
        }

        auto const& dag_entries = dag_entries_expected.value();
        if (dag_entries.empty())
        {
            menu_tab = horizontal_tab_panel(
                {ftxui::Renderer([] {
                    return ftxui::text("No DAGs available") | ftxui::dim;
                })},
                {"empty"});
            return;
        }

        auto dag_names = dag_entries
            | std::views::transform([](auto const& dag) { return dag.value.name; })
            | std::ranges::to<std::vector>();

        ftxui::Components menu_components;
        menu_components.reserve(dag_names.size());

        for (auto const& dag_name : dag_names)
        {
            auto tasks_expected = storage->tasks().list_tasks(dag_name);
            ftxui::Element element{};
            if (not tasks_expected)
            {
                auto message = std::string{"Unable to load tasks for "};
                message.append(dag_name);
                element = ftxui::text(message) | ftxui::color(ftxui::Color::Orange1);
            }
            else
            {
                element = build_dag_task_table(dag_name, tasks_expected.value());
            }

            menu_components.push_back(ftxui::Renderer([element] { return element; }));
        }

        menu_tab = horizontal_tab_panel(std::move(menu_components), std::move(dag_names));
    }

    dag_panel::dag_panel(concurrent_shyguy_t const& shy)
        : shyguy{shy}
    {
        update();
    }
}
