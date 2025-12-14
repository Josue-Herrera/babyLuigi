#include "dagpanel.h"

#include <threadsafe_shyguy/concurrent_shyguy.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include "horizontaltabpanel.h"

namespace cosmos::inline v1
{
    namespace
    {
        auto get_logger()
        {
            auto logger = spdlog::get("shyguy_logger");
            if (not logger)
            {
                logger = spdlog::basic_logger_mt("shyguy_logger", "logs/shyguy.log", true);
            }
            return logger;
        }

        const std::vector<std::string> schedule_names
        {
            "hourly",
            "daily",
            "weekly",
            "monthly",
            "yearly",
            "custom"
        };

        const std::vector schedule_tabs
        {
            schedule_frequency::hourly,
            schedule_frequency::daily,
            schedule_frequency::weekly,
            schedule_frequency::monthly,
            schedule_frequency::yearly,
            schedule_frequency::custom
        };

        // This needs to use the Table Container!
        [[nodiscard]] auto make_components(std::vector<horizontal_tab_panel> &pages) -> ftxui::Components
        {
            ftxui::Components out;
            out.reserve(pages.size());
            for (auto &page : pages)
                out.push_back(page.render());
            return out;
        }


        [[nodiscard]]auto make_pages(const concurrent_shyguy_t& shy, const std::vector<std::string>& schedules)
        {

        }

    } // namespace

    void dag_panel::update()
    {
        const auto logger = get_logger();
        pages.clear();

        if (not shyguy)
            return logger->debug("Shyguy unavailable");

        auto storage = shyguy->storage_handle();
        if (not storage)
            return logger->debug("Storage unavailable");

        const auto dag_entries = storage->dags().list_dags()
        .transform([&](std::vector<dag_entry> const&dags)
        {
            if (dags.empty()) return std::vector<std::string>{};
            return dags
            | std::views::transform([](auto const& dag){return dag.value.name;})
            | std::ranges::to<std::vector>();
        }).value();

        auto dag_pages = dag_entries.value();
        | std::views::transform([](auto const& dag){return dag.value.name;})
        | std::ranges::to<std::vector>();

        auto task_names = dag_pages;
        | std::views::transform([&storage](auto const& dag_name)
        {
            constexpr std::size_t history_columns_size = 15;
            using table_data_t = std::vector<std::vector<std::string>>;
            auto tasks_of_dag = storage->tasks().list_tasks(dag_name);
            if (not tasks_of_dag and not tasks_of_dag->empty())
                return std::optional<table_data_t>{};


            auto values = tasks_of_dag.transform(
            [](std::vector<task_entry> const& tasks)
            {
                return tasks
                | std::views::transform([](auto const& task)
                {
                    return task.value.name;
                });
            });
        });

        auto const menu_components = pages | std::views::transform(
        [](auto const& page)
        {
            return page.render();
        })
        | std::ranges::to<std::vector>();

        menu_tab = cosmos::horizontal_tab_panel(menu_components, dag_entries);
    }

    dag_panel::dag_panel(concurrent_shyguy_t const& shy):
        shyguy{shy}
    {
        update();
    }
}
