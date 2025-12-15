
#include "concurrent_shyguy.hpp"

#include <process/system_execution.hpp>

#include <range/v3/all.hpp>
#include <spdlog/spdlog.h>

#include <cron_parser/cron_expression.hpp>
#include <blocking_priority_queue.hpp>
#include <task_request.hpp>

#include <ranges>
#include <span>

namespace cosmos::inline v1
{
    auto concurrent_shyguy::make_task_metadata(shyguy_task const &task) const -> std::optional<task_metadata>
    {
        std::optional<std::string> cron_expression{};

        if (auto const in_memory = schedules.find(task.associated_dag); in_memory != schedules.end())
        {
            cron_expression = in_memory->second;
        }
        else if (storage)
        {
            auto dag_entry = storage->dags().get_dag(task.associated_dag);
            if (dag_entry and dag_entry->value.schedule and not dag_entry->value.schedule->empty())
                cron_expression = dag_entry->value.schedule.value();
        }

        if (not cron_expression or cron_expression->empty())
            return std::nullopt;

        task_metadata metadata{};
        metadata.schedule.cron_expression = *cron_expression;
        metadata.schedule.frequency = classify_schedule(*cron_expression);
        return metadata;
    }

    auto concurrent_shyguy::process(shyguy_request const &request) noexcept -> command_result_type
    {
        std::lock_guard lock(mutex);
        return request.data | match
               {
                       [this, command = request.command](requestable auto const &dag_or_task) -> command_result_type
                       {
                           switch (command)
                           {
                               case command_enum::create:
                                   return create(dag_or_task);
                               case command_enum::remove:
                                   return remove(dag_or_task);
                               case command_enum::execute:
                                   return execute(dag_or_task);
                               case command_enum::snapshot:
                                   return snapshot(dag_or_task);
                               default:
                                   return {std::unexpected(command_error::unknown_command)};
                           }
                       },
                       [this](std::monostate const m) { return process(m); }
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

        // persist into storage if available
        if (storage)
        {
            (void) storage->dags().upsert_dag(dag, std::nullopt);
        }
        return log_return("created dag {}", dag.name);
    }

    auto concurrent_shyguy::remove(shyguy_dag const &dag) noexcept -> command_result_type
    {
        if (bool const erased = dags.erase(dag.name); erased)
        {
            if (has_schedule(dag))
                schedules.erase(dag.name);

            // remove from storage if available
            if (storage)
            {
                (void) storage->dags().erase_dag(dag.name, std::nullopt);
            }
            return log_return("removed dag {}", dag.name);
        }

        return std::unexpected(command_error::dag_deletion_failed);
    }

    auto concurrent_shyguy::execute(shyguy_dag const &dag) noexcept -> command_result_type
    {
        return execute_at(dag, std::chrono::steady_clock::now());
    }

    auto concurrent_shyguy::execute_at(shyguy_dag const& dag, std::chrono::steady_clock::time_point scheduled_time) noexcept
        -> command_result_type
    {
        std::lock_guard lock(mutex);
        auto const dag_iter = dags.find(dag.name);
        if (dag_iter == end(dags))
            return std::unexpected(command_error::dag_not_found);

        auto const ordered_tasks = dag_iter->second.run_order();

        if (not ordered_tasks)
            return std::unexpected(command_error::task_creates_cycle);

        auto const &order = ordered_tasks.value();

        auto tr = std::make_shared<task_request>(task_request{
            .scheduled_time = scheduled_time,
            .sequence = task_request_sequence.fetch_add(1U, std::memory_order_relaxed),
            .payload = task_request_payload{
            std::views::transform(order, [this, &dag](auto const& task_name)
            {
                task_runner runner{};
                runner.name = task_name;
                if (storage)
                {
                    auto rv = storage->tasks().get_task(dag.name, task_name);
                    if (rv && rv.value().value.file_content.has_value())
                        runner.contents = rv.value().value.file_content.value();
                }

                runner.task_function = [this]() noexcept -> void
                {
                    std::lock_guard lock(mutex);
                    std::string const command{"./task_executable"};
                    if (auto output = execute_command(command); output)
                        logger->info("Task executed successfully with output: {}", output.value());
                    else
                        logger->error("Task execution failed with exit code: {}", output.error());
                };

                return runner;
            }) | ranges::v3::to<std::vector>(),
            dag_iter->second
        }});

        request_queue->enqueue(std::move(tr));

        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::snapshot(shyguy_dag const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::create(shyguy_task const &task) noexcept -> command_result_type
    {
        task_map.insert_or_assign(task.name, task);
        auto &dag = dags.at(task.associated_dag);
        if (auto const inserted = dag.push_task(task.name, task.dependency_names); not inserted)
            return std::unexpected(static_cast<command_error>(inserted.error()));

        auto metadata = storage ? make_task_metadata(task) : std::optional<task_metadata>{};

        if (task.file_content and task.filename)
        {
            // also persist to storage
            if (storage)
            {
                const auto bytes = std::as_bytes(
                        std::span{std::data(*task.file_content), std::size(*task.file_content)});
                if (auto id = storage->blobs().put_blob(bytes))
                {
                    (void) storage->tasks().upsert_task(task, *id, std::nullopt, metadata);
                }
                else
                {
                    spdlog::warn("blob store put failed for task {}", task.name);
                    (void) storage->tasks().upsert_task(task, std::nullopt, std::nullopt, metadata);
                }
            }
        }
        else
        {
            if (storage)
            {
                (void) storage->tasks().upsert_task(task, std::nullopt, std::nullopt, metadata);
            }
        }

        return log_return("Created New Task {}", task.name);
    }

    auto concurrent_shyguy::remove(shyguy_task const &task) noexcept -> command_result_type
    {
        if (auto const dag = dags.find(task.associated_dag); dag != std::end(dags))
        {
            if (auto const removed = dag->second.remove_task(task.name); not removed)
                return std::unexpected(static_cast<command_error>(removed.error()));

        }

        if (storage)
        {
            (void) storage->tasks().erase_task(task.associated_dag, task.name);
        }
        return log_return("Removed Task {}", task.name);
    }

    auto concurrent_shyguy::execute(shyguy_task const &task) noexcept -> command_result_type
    {
        using namespace std::string_literals;
        std::lock_guard lock(mutex);
        auto const dag = dags.find(task.associated_dag);
        if (dag == std::end(dags))
            return std::unexpected(command_error::dag_not_found);

        auto const ordered_tasks = dag->second.run_order();
        if (not ordered_tasks)
            return std::unexpected(command_error::task_creates_cycle);

        auto const &order = ordered_tasks.value();


        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::snapshot(shyguy_task const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::process(std::monostate) const noexcept -> command_result_type
    {
        logger->info("Monostate input. This should never happen.");
        return std::unexpected(command_error::monostate_reached);
    }

    auto concurrent_shyguy::next_scheduled_dag() const noexcept -> std::optional<notification_type>
    {
        std::lock_guard lock(this->mutex);

        if (schedules.empty())
            return std::nullopt;

        using system_clock = std::chrono::system_clock;
        auto const now_system = system_clock::now();

        std::optional<std::pair<std::string, system_clock::time_point>> next{};
        for (auto const& [dag_name, cron_expr] : schedules)
        {
            try
            {
                geheb::cron_expression cron{cron_expr};
                auto const tp = cron.calc_next(now_system);
                if (not next || tp < next->second)
                    next = std::pair{dag_name, tp};
            }
            catch (...)
            {
                // Ignore invalid schedule in-memory.
            }
        }

        if (not next)
            return std::nullopt;

        auto const wait = next->second - now_system;
        auto const now_steady = std::chrono::steady_clock::now();

        shyguy_request request{};
        request.command = command_enum::execute;
        request.emplace(shyguy_dag{.name = next->first, .schedule = schedules.at(next->first)});

        return notification_type{
            .time = now_steady + std::chrono::duration_cast<std::chrono::steady_clock::duration>(wait),
            .associated_request = std::move(request),
            .command_type = command_enum::execute,
        };
    }
}
