//
// Created by jaxma on 10/4/2025.
//
#include "dagpage.h"

#include <threadsafe_shyguy/concurrent_shyguy.hpp>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace cosmos::inline v1
{
    namespace
    {
        using dag_rows = std::vector<task_entry>;
        constexpr std::size_t run_history_columns = 15;

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

        [[nodiscard]] auto schedule_label(schedule_frequency frequency) -> std::string
        {
            auto view = to_string(frequency);
            return {view.begin(), view.end()};
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

        [[nodiscard]] auto format_timestamp(std::chrono::system_clock::time_point tp) -> std::string
        {
            auto const time = std::chrono::system_clock::to_time_t(tp);
            std::tm tm{};
    #if defined(_WIN32)
            localtime_s(&tm, &time);
    #else
            localtime_r(&time, &tm);
    #endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%m-%d %H:%M");
            if (oss.fail())
                return "n/a";
            return oss.str();
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
                auto const tp = now - interval * offset;
                labels.emplace_back(format_timestamp(tp));
            }
            return labels;
        }
    } // namespace

    dag_page::dag_page(schedule_frequency schedule, concurrent_shyguy_t const& shy):
        frequency{schedule}
    {
        content = ftxui::Menu(&names, &selected_dag);
        renderer = ftxui::Renderer([&]
        {
            return ftxui::hbox(
                content->Render(),
                ftxui::separator(),
                render(shy)
            );
        });
    }

    auto dag_page::render(concurrent_shyguy_t shyguy) -> ftxui::Element
    {
        if (not shyguy)
            return ftxui::text("Shyguy unavailable") | ftxui::color(ftxui::Color::RedLight);

        auto storage = shyguy->storage_handle();
        if (not storage)
            return ftxui::text("Storage unavailable") | ftxui::dim;

        auto dag_entries_expected = storage->dags().list_dags();
        if (not dag_entries_expected)
            return ftxui::text("Unable to load DAG metadata") | ftxui::color(ftxui::Color::Orange1);

        auto const& dag_entries = dag_entries_expected.value();
        if (dag_entries.empty())
            return ftxui::text("No DAGs available") | ftxui::dim;

        names = dag_entries | std::views::transform([](auto const& dag)
        {
            return dag.value.name;
        })
        | std::ranges::to<std::vector>();

        auto const selected_index_ptr = &selected_dag;
        auto const raw_index = selected_index_ptr ? *selected_index_ptr : 0;
        auto const safe_index = std::clamp(raw_index, std::int32_t{0}, static_cast<std::int32_t>(dag_entries.size() - 1));
        auto const& dag_name = dag_entries.at(static_cast<std::size_t>(safe_index)).value.name;

        auto tasks_of_dag = storage->tasks().list_tasks(dag_name);
        if (not tasks_of_dag)
            return ftxui::text("Unable to load Task data from dag");

        auto const& tasks = tasks_of_dag.value();
        if (tasks.empty())
        {
            auto message = std::string{"No tasks registered for "};
            message += dag_name;
            return ftxui::text(message) | ftxui::dim;
        }

        auto const run_labels = make_run_labels(frequency, run_history_columns);
        std::vector<std::vector<std::string>> table_rows;
        table_rows.reserve(tasks.size() + 1);

        std::vector<std::string> header_row;
        header_row.reserve(run_history_columns + 1);
        auto header_title = dag_name;
        header_title.append(" [");
        header_title.append(schedule_label(frequency));
        header_title.append("]");
        header_row.push_back(std::move(header_title));
        header_row.insert(header_row.end(), run_labels.begin(), run_labels.end());
        table_rows.push_back(std::move(header_row));

        std::vector<std::vector<dag_run_status>> status_matrix;
        status_matrix.reserve(tasks.size());
        constexpr auto status_cell_fill = "     ";

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

            for (std::size_t column = 0; column < run_history_columns; ++column)
                row.emplace_back(status_cell_fill);

            table_rows.push_back(std::move(row));
            status_matrix.push_back(std::move(statuses));
        }

        ftxui::Table table{table_rows};

        table.SelectAll().Border(ftxui::LIGHT);
        table.SelectRow(0).Decorate(ftxui::bold);
        table.SelectRow(0).Decorate(ftxui::bgcolor(ftxui::Color::GrayDark));
        table.SelectRow(0).Decorate(ftxui::color(ftxui::Color::White));
        table.SelectColumn(0).Decorate(ftxui::bold);
        table.SelectColumn(0).Decorate(ftxui::bgcolor(ftxui::Color::GrayLight));
        table.SelectColumn(0).Decorate(ftxui::color(ftxui::Color::Black));

        for (std::size_t row_index = 0; row_index < status_matrix.size(); ++row_index)
        {
            auto const& statuses = status_matrix[row_index];
            for (std::size_t column_index = 0; column_index < statuses.size(); ++column_index)
            {
                auto selection = table.SelectCell(static_cast<int>(column_index + 1), static_cast<int>(row_index + 1));
                selection.Decorate(ftxui::bgcolor(status_color(statuses[column_index])));
                selection.Decorate(ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 5));
                selection.Decorate(ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1));
            }
        }

        return table.Render();
    }
}
