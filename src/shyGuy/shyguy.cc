#include "shyguy.hpp"

// *** Project Includes ***
#include "fs_storage.hpp"
#include <shyguy_request.hpp>
#include "concurrent_shyguy.hpp"
#include "zmq_router.hpp"
#include "tui.h"
#include "shyexecutioner.hpp"
#include "blocking_priority_queue.hpp"
#include "task_request.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexec/execution.hpp>

// *** Standard ***
#include <unordered_map>

namespace cosmos::inline v1
{
    auto root_folder()
    {
#ifdef _WIN32
        if (auto const app_data = std::getenv("COSMOS_DATA"))
            return std::filesystem::path(app_data);
        else
            return std::filesystem::path("C:/ProgramData");
#else
        auto app_data = std::getenv("$COSMOS_HOME");
        return std::filesystem::path(app_data);
#endif
    }

    auto shyguy::run() noexcept -> void
    {
        auto file_logger = spdlog::basic_logger_mt("shyguy_logger", "logs/shy-log.txt", true);
        auto request_queue = std::make_shared<blocking_priority_queue<task_request_ptr, task_request_ptr_compare>>();
        auto io_queue      = std::make_shared<blocking_queue<shyguy_request>>();
        auto store         = std::make_shared<fs_storage>(root_folder());
        auto terminator    = std::make_shared<std::atomic_bool>(true);
        auto shyguy        = std::make_shared<concurrent_shyguy>(request_queue, terminator, store);


        auto high_level_pool = exec::static_thread_pool{3};
        auto high_level_scheduler = high_level_pool.get_scheduler();
        auto scope = exec::async_scope{};

        notify_updater notifier{};
        std::mutex execution_mutex{};
        std::atomic_bool execution_running{false};
        std::optional<std::thread> execution_thread{};

        auto ensure_executioner_running = [&]
        {
            if (not terminator->load(std::memory_order_relaxed))
                return;

            std::lock_guard lk(execution_mutex);
            if (execution_thread && execution_thread->joinable() && not execution_running.load(std::memory_order_relaxed))
            {
                execution_thread->join();
                execution_thread.reset();
            }

            if (execution_running.load(std::memory_order_relaxed))
                return;

            execution_running.store(true, std::memory_order_relaxed);
            execution_thread.emplace([&]
            {
                shy_executioner exec{terminator, request_queue};
                exec.run_until_idle(std::chrono::milliseconds{arguments.execution_idle_ms},
                                    arguments.max_dag_concurrency,
                                    arguments.max_task_concurrency);
                execution_running.store(false, std::memory_order_relaxed);
            });
        };

        std::thread request_thread{[&, io_queue]
        {
            using namespace std::chrono_literals;
            while (terminator->load(std::memory_order_relaxed))
            {
                auto request = io_queue->dequeue_wait(50ms);
                if (not request)
                    continue;

                (void) shyguy->process(*request);
                if (auto next = shyguy->next_scheduled_dag(); next.has_value())
                    notifier.notify_wakeup(next.value());

                ensure_executioner_running();
            }
        }};

        std::thread schedule_thread{[&]
        {
            using namespace std::chrono_literals;
            std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_enqueued{};
            while (terminator->load(std::memory_order_relaxed))
            {
                auto next = shyguy->next_scheduled_dag();
                if (not next)
                {
                    (void) notifier.sleep_until_or_notified(std::chrono::steady_clock::now() + std::chrono::months(1));
                    continue;
                }

                auto const& scheduled = *next;
                auto dag_ptr = std::get_if<shyguy_dag>(&scheduled.associated_request.data);
                if (dag_ptr)
                {
                    auto& last = last_enqueued[dag_ptr->name];
                    if (last != scheduled.time)
                    {
                        last = scheduled.time;
                        (void) shyguy->execute_at(*dag_ptr, scheduled.time);
                        ensure_executioner_running();
                    }
                }

                if (auto notification = notifier.sleep_until_or_notified(scheduled.time); notification.has_value())
                    continue;
            }
        }};

        if (auto next = shyguy->next_scheduled_dag(); next.has_value())
            notifier.notify_wakeup(next.value());

        auto interactive_view =
        stdexec::starts_on(high_level_scheduler,
           stdexec::just(arguments.interactive)
           | stdexec::then([&](bool interactive)
           {
               file_logger->info("{}", interactive);
               if (interactive)
               {
                   tui view{};
                   view.run(shyguy, io_queue);
                   terminator->store(false, std::memory_order_relaxed);
               }
           })
        );

        scope.spawn(std::move(interactive_view));

        (void) stdexec::sync_wait(scope.on_empty());
        high_level_pool.request_stop();

        terminator->store(false, std::memory_order_relaxed);
        notifier.notify_wakeup({.time = std::chrono::steady_clock::now(),
                                .associated_request = {},
                                .command_type = command_enum::error});
        if (request_thread.joinable())
            request_thread.join();
        if (schedule_thread.joinable())
            schedule_thread.join();
        if (execution_thread && execution_thread->joinable())
            execution_thread->join();

    }

    auto shyguy::test_run() noexcept -> void
    {
        auto file_logger = spdlog::basic_logger_mt("shyguy_logger", "logs/shy-log.txt", true);
        auto request_queue = std::make_shared<blocking_priority_queue<task_request_ptr, task_request_ptr_compare>>();
        auto terminator = std::make_shared<std::atomic_bool>(false);
        auto shyguy = concurrent_shyguy{request_queue, terminator, storage_};
        auto router = zmq_router{request_queue, terminator};

        auto io_queue = std::make_shared<blocking_queue<shyguy_request>>();
        auto cosmos_pool = exec::static_thread_pool{std::thread::hardware_concurrency()};
        auto cosmos_scheduler = cosmos_pool.get_scheduler();

        auto io_pool = exec::static_thread_pool{1};
        auto io_scheduler = io_pool.get_scheduler();

        auto scope = exec::async_scope{};

        while (terminator->load(std::memory_order_relaxed))
        {
            auto payload_handler = stdexec::just() | stdexec::then([&router]() mutable
            {
                return router.route_data();
            });

            auto io_handler =
                    stdexec::starts_on(io_scheduler, std::move(payload_handler))
                    | stdexec::continues_on(cosmos_scheduler)
                    | stdexec::then([&shyguy](std::optional<shyguy_request> request)
                    {
                        if (request)
                            shyguy.process(std::move(*request));
                    });

            scope.spawn(std::move(io_handler));
        }

        (void) stdexec::sync_wait(scope.on_empty());
        cosmos_pool.request_stop();
        io_pool.request_stop();
    };
}
