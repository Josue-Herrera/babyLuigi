// *** Project Includes ***
#include "shyexecutioner.hpp"
#include "blocking_queue.hpp"
#include "shyguy_request.hpp"
#include "process/system_execution.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <range/v3/view/chunk.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <graph/graph.hpp>



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

    auto shy_executioner::process() const noexcept -> void
    {
        using namespace std::chrono_literals;
        const auto logger = get_logger();
        auto high_level_pool      = exec::static_thread_pool{4};
        auto high_level_scheduler = high_level_pool.get_scheduler();
        auto scope                = exec::async_scope{};

        while (running->load(std::memory_order_relaxed))
        {
            const auto [task_runners, dag] = request_queue->dequeue();
            bool available_tasks_to_run = true;
            while (available_tasks_to_run and running->load(std::memory_order_relaxed))
            {
                std::vector<std::reference_wrapper<const task_runner>> task_refs { std::begin(task_runners), std::end(task_runners) };
                for(auto const& task_group : task_refs | ranges::views::chunk(4))
                {
                    for (auto const& tasks : task_group)
                    {
                        auto sender = stdexec::just(tasks.get(), std::make_shared<directed_acyclic_graph>(dag))
                            | stdexec::then([logger] (auto const& r, const std::shared_ptr<directed_acyclic_graph> &d)
                            {
                                logger->info( "[shy_exec]Processing task: {} in DAG: {}", r.name, d->root_view());

                            });

                        scope.spawn(stdexec::on(high_level_scheduler, std::move(sender)));
                    }
                }


            }
        }

        stdexec::sync_wait(scope.on_empty());
    }

    auto shy_executioner::dispatcher() noexcept -> void
    {

        const auto logger = get_logger();
        while (running->load(std::memory_order_relaxed))
        {
           process();
        }

    }
}
