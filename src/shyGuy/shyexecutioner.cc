// *** Project Includes ***
#include "shyexecutioner.hpp"
#include "blocking_queue.hpp"
#include "blocking_priority_queue.hpp"
#include "process/system_execution.hpp"
#include "shyguy_request.hpp"
#include "task_request.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>

// *** Standard Includes ***
#include <algorithm>
#include <chrono>
#include <cstdint>
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

    auto shy_executioner::run_until_idle(std::chrono::milliseconds idle_timeout,
                                         std::size_t max_dag_concurrency,
                                         std::size_t max_task_concurrency) noexcept -> void
    {
        using task_index = std::unordered_map<std::string, std::reference_wrapper<task_runner>>;
        using name_set   = std::unordered_set<std::string>;

        using namespace std::chrono_literals;
        const auto logger         = get_logger();
        auto const dag_threads = static_cast<std::uint32_t>(std::max<std::size_t>(1, max_dag_concurrency));
        auto const task_threads = static_cast<std::uint32_t>(std::max<std::size_t>(1, max_task_concurrency));

        auto dag_pool             = exec::static_thread_pool{dag_threads};
        auto dag_scheduler        = dag_pool.get_scheduler();
        auto task_pool            = exec::static_thread_pool{task_threads};
        auto task_scheduler       = task_pool.get_scheduler();
        auto dag_scope            = exec::async_scope{};

        std::atomic_uint32_t in_flight_dags{0};

        auto select_ready_tasks = [&](const directed_acyclic_graph &dag, 
                                      const name_set &pending,
                                      const name_set &completed) -> std::vector<std::string>
        {
            std::vector<std::string> ready;
            ready.reserve(max_task_concurrency);
            for (auto const &name: pending)
            {
                if (dag.is_task_ready(completed, name))
                {
                    ready.push_back(name);
                    if (ready.size() >= max_task_concurrency)
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

                wave_scope.spawn(stdexec::on(task_scheduler, std::move(sender)));
            }

            stdexec::sync_wait(wave_scope.on_empty());
            return finished_names;
        };

        auto run_dependency_waves = [&](const directed_acyclic_graph &dag, std::vector<task_runner> runners)
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

        auto last_work = std::chrono::steady_clock::now();
        while (running->load(std::memory_order_relaxed))
        {
            auto request = request_queue->dequeue_wait(idle_timeout);
            if (not request)
            {
                if (in_flight_dags.load(std::memory_order_relaxed) == 0U
                    && (std::chrono::steady_clock::now() - last_work) >= idle_timeout)
                {
                    break;
                }
                continue;
            }

            last_work = std::chrono::steady_clock::now();

            in_flight_dags.fetch_add(1U, std::memory_order_relaxed);
            auto sender = stdexec::just(std::move(*request))
                | stdexec::then([&](task_request_ptr tr)
                {
                    try
                    {
                        if (not tr)
                            return;

                        auto& [task_runners, dag] = tr->payload;
                        run_dependency_waves(dag, std::move(task_runners));
                    }
                    catch (std::exception const& e)
                    {
                        logger->error("[shy_exec] DAG runner threw exception: {}", e.what());
                    }
                    catch (...)
                    {
                        logger->error("[shy_exec] DAG runner threw unknown exception");
                    }

                    in_flight_dags.fetch_sub(1U, std::memory_order_relaxed);
                });

            dag_scope.spawn(stdexec::on(dag_scheduler, std::move(sender)));
        }

        stdexec::sync_wait(dag_scope.on_empty());
        dag_pool.request_stop();
        task_pool.request_stop();
    }
} // namespace cosmos::inline v1
