// *** Project Includes ***
#include "shyexecutioner.hpp"
#include "blocking_queue.hpp"
#include "process/system_execution.hpp"
#include "shyguy_request.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <chrono>
#include <graph/graph.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace cosmos::inline v1
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

    auto shy_executioner::dag_processing_factory_line() const noexcept -> void
    {
        static constexpr const std::size_t max_concurrency = 4;
        using task_index = std::unordered_map<std::string, std::reference_wrapper<task_runner>>;
        using name_set   = std::unordered_set<std::string>;

        using namespace std::chrono_literals;
        const auto logger         = get_logger();
        auto high_level_pool      = exec::static_thread_pool{max_concurrency};
        auto high_level_scheduler = high_level_pool.get_scheduler();

        auto select_ready_tasks = [&](const directed_acyclic_graph &dag, 
                                      const name_set &pending,
                                      const name_set &completed) -> std::vector<std::string>
        {
            std::vector<std::string> ready;
            ready.reserve(max_concurrency);
            for (auto const &name: pending)
            {
                if (dag.is_task_ready(completed, name))
                {
                    ready.push_back(name);
                    if (ready.size() >= max_concurrency)
                        break;
                }
            }
            return ready;
        };

        auto run_wave = [&](const directed_acyclic_graph &dag, 
                            task_index &by_name,
                            std::vector<std::string> const &ready_names) -> std::vector<std::string>
        {
            auto wave_scope = exec::async_scope{};
            std::mutex wave_mutex;
            std::vector<std::string> finished_names;
            finished_names.reserve(ready_names.size());

            for (auto const &name: ready_names)
            {
                auto &tr = by_name.at(name);
                auto sender =
                        stdexec::just(tr) |
                        stdexec::then(
                                [&](task_runner &r)
                                {
                                    logger->info("[shy_exec] Starting task: {} in DAG: {}", r.name, dag.view_name());
                                    try
                                    {
                                        if (r.task_function)
                                            r.task_function();
                                        else
                                            logger->warn("[shy_exec] Task '{}' has no function to run", r.name);
                                    }
                                    catch (std::exception const &e)
                                    {
                                        logger->error("[shy_exec] Task '{}' threw exception: {}", r.name, e.what());
                                    }
                                    catch (...)
                                    {
                                        logger->error("[shy_exec] Task '{}' threw unknown exception", r.name);
                                    }
                                    logger->info("[shy_exec] Finished task: {} in DAG: {}", r.name, dag.view_name());
                                    return r.name;
                                }) |
                        stdexec::then(
                                [&](std::string finished_name)
                                {
                                    std::lock_guard lk(wave_mutex);
                                    finished_names.push_back(std::move(finished_name));
                                });

                wave_scope.spawn(stdexec::on(high_level_scheduler, std::move(sender)));
            }

            stdexec::sync_wait(wave_scope.on_empty());
            return finished_names;
        };

        auto run_dependency_waves = [&](const directed_acyclic_graph &dag, std::vector<task_runner> &runners)
        {
            task_index by_name;
            by_name.reserve(runners.size());
            for (auto &tr: runners) by_name.emplace(tr.name, tr);
            
            name_set pending;
            pending.reserve(by_name.size());
            for (auto const &[name, _]: by_name) pending.insert(name);
            
            name_set completed;
            completed.reserve(by_name.size());

            while (not pending.empty() and running->load(std::memory_order_relaxed))
            {
                auto ready = select_ready_tasks(dag, pending, completed);
                if (ready.empty())
                {
                    logger->error("[shy_exec] No ready tasks but pending remain in DAG: {}. Possible dependency issue.",
                                  dag.view_name());
                    break;
                }

                auto finished = run_wave(dag, by_name, ready);
                for (auto &name: finished)
                {
                    completed.insert(name);
                    pending.erase(name);
                }
            }
        };

        while (running->load(std::memory_order_relaxed))
        {
            auto [task_runners, dag] = request_queue->dequeue();
            run_dependency_waves(dag, task_runners);
        }
        
    }

    auto shy_executioner::dispatcher() noexcept -> void
    {

        const auto logger = get_logger();
        while (running->load(std::memory_order_relaxed))
        {
            dag_processing_factory_line();
        }
    }
} // namespace cosmos::inline v1
