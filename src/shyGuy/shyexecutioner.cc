// *** Project Includes ***
#include "shyexecutioner.hpp"
#include "blocking_queue.hpp"
#include "shyguy_request.hpp"

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
        auto high_level_pool      = exec::static_thread_pool{3};
        auto high_level_scheduler = high_level_pool.get_scheduler();
        auto scope                = exec::async_scope{};

        const auto rv = request_queue->dequeue_wait(1min);

        if (not rv)
        {
            logger->info("[shy_exec]No tasks to process, waiting for next task...");
            return;
        }

        auto const&  [task_runners, dag] = *rv;

        for(auto const& task_group : task_runners | ranges::views::chunk(4))
        {
            for (auto const& tasks : task_group )
            {
                auto sender = stdexec::just(tasks, dag)
                    | stdexec::then([logger] (auto const& r, auto const& d)
                    {
                        logger->info( "[shy_exec]Processing task: {} in DAG: {}", r.name, d.root_view());
                    });

                scope.spawn(stdexec::on(high_level_scheduler, std::move(sender)));
                stdexec::sync_wait(scope.on_empty());
            }
        }
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
