
#include <spdlog/spdlog.h>
#include <uuid.h>
#include <stdexec/execution.hpp>

#include <any>

#include "concurrent_shyguy.hpp"


namespace cosmos::inline v1
{
    auto concurrent_shyguy::process(command_enum type, shyguy_request const &request) noexcept -> command_result_type
    {
        std::lock_guard lock(mutex);
        return request.data | match
        {
            [this, type](requestable auto const& dag_or_task) -> command_result_type
            {
                switch (type)
                {
                    case command_enum::create: return create(dag_or_task);
                    case command_enum::remove: return remove(dag_or_task);
                    case command_enum::execute: return execute(dag_or_task);
                    case command_enum::snapshot: return snapshot(dag_or_task);
                    default: return { std::unexpected(command_error::unknown_command) };
                }
            },
            [this, type](std::monostate const m) { return process(type, m); }
        };
    }

    auto concurrent_shyguy::create(shyguy_dag const &dag) noexcept -> command_result_type
    {
        if (auto const dag_found = dags.find(dag.name); dag_found != dags.end())
            return std::unexpected(command_error::dag_duplicate);

        if (auto [_, in] = dags.emplace(dag.name, directed_acyclic_graph(dag.name)); not in)
            return std::unexpected(command_error::dag_insertion_failed);

        if (has_schedule(dag))
            schedules.emplace(dag.name, dag.schedule.value());

        return log_return("created dag {}", dag.name);
    }

    auto concurrent_shyguy::remove(shyguy_dag const &dag) noexcept -> command_result_type
    {
        if (bool const erased = dags.erase(dag.name); erased)
        {
            if (has_schedule(dag))
                schedules.erase(dag.name);

           return log_return("removed dag {}", dag.name);
        }

        return std::unexpected(command_error::dag_deletion_failed);
    }

    auto concurrent_shyguy::execute(shyguy_dag const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::snapshot(shyguy_dag const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::create(shyguy_task const &task) noexcept -> command_result_type
    {
        auto &dag = dags.at(task.associated_dag);
        if (auto const inserted = dag.push_task(task.name, task.dependency_names); !inserted)
            return std::unexpected(static_cast<command_error>(inserted.error()));

        return log_return("Created New Task {}", task.name);
    }

    auto concurrent_shyguy::remove(shyguy_task const& task) noexcept -> command_result_type
    {
        if (auto const dag = dags.find(task.associated_dag); dag != end(dags))
        {
            if (auto const removed = dag->second.remove_task(task.name); !removed)
                return std::unexpected(static_cast<command_error>(removed.error()));

        }
        return log_return("Removed Task {}", task.name);
    }

    inline auto create_dag_run_folder(std::filesystem::path const& app = {}) -> std::filesystem::path
    {
        // Get current date
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);

        // Format: year_month_day
        std::string date_folder = std::format
        (
            "{:04}_{:02}_{:02}",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday
        );

        // Generate UUID
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size> {};
        std::ranges::generate(seed_data, std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};
        const auto uuid = gen();
        std::string uuid_str = uuids::to_string(uuid);

        // Build full path: appData/year_month_day/unique_id
        auto app_data = app;
        if (app_data.empty())
            app_data = getenv("APPDATA");

        auto run_folder = app_data / "cosmos" / "shyguy" / date_folder / uuid_str;

        if (std::filesystem::exists(run_folder))
            return run_folder;

        std::filesystem::create_directories(run_folder);
        return run_folder;
    }

    auto concurrent_shyguy::execute(shyguy_task const &task) noexcept -> command_result_type
    {
        auto const run_folder = create_dag_run_folder();
        auto const dag             = dags.find(task.associated_dag);
        if (dag == end(dags))
            return std::unexpected(command_error::dag_not_found);

        auto const ordered_tasks = dag->second.run_order();
        if (not ordered_tasks)
            return std::unexpected(command_error::task_creates_cycle);

        auto const& order = ordered_tasks.value();



        return std::unexpected(command_error::not_currently_supported);
    }
    auto concurrent_shyguy::snapshot(shyguy_task const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::process(command_enum, std::monostate) const noexcept -> command_result_type
    {
        logger->info("Monostate input. This should never happen.");
        return std::unexpected(command_error::monostate_reached);
    }


    auto concurrent_shyguy::next_scheduled_dag() const noexcept -> std::optional<notification_type>
    {
        std::lock_guard lock(this->mutex);
        return
        {
            notification_type
            {
                .time = std::chrono::steady_clock::now() + std::chrono::seconds{5},
                .associated_request = {},
                .command_type = command_enum::execute
            }
        };
    }
}


