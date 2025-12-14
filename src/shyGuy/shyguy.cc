#include "shyguy.hpp"

// *** Project Includes ***
#include "fs_storage.hpp"
#include <shyguy_request.hpp>
#include "concurrent_shyguy.hpp"
#include "zmq_router.hpp"
#include "tui.h"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexec/execution.hpp>

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
        auto request_queue = std::make_shared<blocking_queue<std::pair<std::vector<task_runner>, directed_acyclic_graph>>>();
        auto store         = std::make_shared<fs_storage>(root_folder());
        auto terminator    = std::make_shared<std::atomic_bool>(false);
        auto shyguy        = std::make_shared<concurrent_shyguy>(request_queue, terminator, store);


        // auto io_queue             = std::make_shared<blocking_queue<shyguy_request>>();
        auto high_level_pool = exec::static_thread_pool{3};
        auto high_level_scheduler = high_level_pool.get_scheduler();
        auto scope = exec::async_scope{};

        auto interactive_view =
        stdexec::starts_on(high_level_scheduler,
           stdexec::just(arguments.interactive)
           | stdexec::then([&](bool interactive)
           {
               file_logger->info("{}", interactive);
               if (interactive)
               {
                   tui view{};
                   view.run(shyguy);
               }
           })
        );

        scope.spawn(std::move(interactive_view));

        (void) stdexec::sync_wait(scope.on_empty());
        high_level_pool.request_stop();

    }

    auto shyguy::test_run() noexcept -> void
    {
        auto file_logger = spdlog::basic_logger_mt("shyguy_logger", "logs/shy-log.txt", true);
        auto request_queue = std::make_shared<blocking_queue<std::pair<
            std::vector<task_runner>, directed_acyclic_graph>>>();
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