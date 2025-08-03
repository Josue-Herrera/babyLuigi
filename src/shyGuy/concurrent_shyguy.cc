
#include <spdlog/spdlog.h>

#include "concurrent_shyguy.hpp"


namespace cosmos::inline v1
{
    auto concurrent_shyguy::process(command_enum type, shyguy_request const &request) noexcept -> command_result_type
    {
        std::lock_guard lock(mutex);
        return request.data | match
        {
            [this, type]<typename Request>(Request&& dag_or_task)
            {
               return process(type, std::forward<Request>(dag_or_task));
            }
        };
    }

    auto concurrent_shyguy::process(command_enum type, shyguy_dag const &dag) noexcept -> command_result_type
    {
        switch (type)
        {
            case command_enum::create:
            {
                auto dag_found = dags.find(dag.name);
                if (dag_found == dags.end())
                {
                    dags.emplace(dag.name, directed_acyclic_graph(dag.name));

                    if (has_schedule(dag))
                        schedules.emplace(dag.name, dag.schedule.value());

                    log->info("create dag {}", dag.name);
                }
                else
                {
                    log->info("dag already exists {}", dag.name);
                }
            }
            case command_enum::remove:
            {
                if (bool const erased = dags.erase(dag.name); erased)
                {
                    if (has_schedule(dag))
                        schedules.erase(dag.name);

                    log->info("removed dag {}", dag.name);
                }
                else
                {
                    log->info("dag failed to remove {}", dag.name);
                }

                break;
            }
            case command_enum::execute:
            {
                // find dag, check if its running
                if (const auto dag_iter = dags.find(dag.name); dag_iter == dags.end())
                {
                    log->info("dag does not exist, cannot run {}", dag.name);
                    break;
                }

                log->info("attempting to execute dag {}", dag.name);

                //


                break;
            }
            case command_enum::snapshot:
            {
                break;
            }
            default:
            {
                // log->info("Command Not Supported: {} ", to_string(input));
            }
        }

        return {};
    }

    auto concurrent_shyguy::process(command_enum type, shyguy_task const& task) noexcept -> command_result_type
    {
        switch (type)
        {
            case command_enum::create:
            {
                auto& dag = dags.at(task.associated_dag);
                if (auto const inserted = dag.push_task(task.name, task.dependency_names); inserted)
                {
                    log->info("Create New Task Failed: {} {}",task.name, to_cstring(inserted.error()));
                }
                break;
            }
            case command_enum::remove:
            {
                auto dag = dags.find(task.associated_dag);
                if (dag != dags.end())
                {
                    if (auto removed = dag->second.remove_task(task.name); !removed)
                    {
                        log->info("Removed Task Failed {} {}",task.name, to_cstring(removed.error()));
                    }
                }
                break;
            }
            case command_enum::execute:
            {
                break;
            }
            case command_enum::snapshot:
            {
                break;
            }
            default:
            {
                log->info("Task Command Not Supported: {} ", to_string_view(type));
            }
        }

        return {};
    }

    auto concurrent_shyguy::process(command_enum, std::monostate) const noexcept -> command_result_type
    {
        log->info("Monostate input. This should never happen.");
        return {
            .message = "Monostate reached. This should never happen.",
            .result = command_result_type::error
        };
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


