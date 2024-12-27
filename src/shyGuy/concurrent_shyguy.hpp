#pragma once

// *** Project Includes ***
#include "graph/graph.hpp"
#include "shyguy_request.hpp"
#include "blocking_queue.hpp"

// *** 3rd Party Includes ***
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>

// *** Standard Includes ***
#include <filesystem>

namespace cosmos::inline v1 {
    // support API commands
    struct command {
        enum type : uint8_t {
            create,
            remove,
            execute,
            snapshot,
            command_error
        };

        type command_{};
    };

    struct command_result_type {

    };
    struct notification_type {
        std::chrono::steady_clock::time_point time;
        cosmos::shyguy_request associated_request;
        command::type command_type_;
    };

    class concurrent_shyguy {
    public:
        /**
         * @brief Create DAG in a concurrent manner
         *
         * @param input
         * @param request
         */
        auto process(command::type const input, cosmos::shyguy_request const &request) noexcept -> command_result_type {
            std::lock_guard lock(this->mutex);
            this->log->info("{}, {}", std::to_underlying(input), request.name);

            switch (input) {
                case command::type::create: {
                    if (is_dag(request) and unique_name(request) and has_schedule(request)) {
                        this->dags.emplace(request.name, directed_acyclic_graph(request.name));
                        this->dags.emplace(request.name, request.schedule.value());
                        this->log->debug("create dag {} with schedule {}",
                            request.name, request.schedule.value()
                        );

                    }

                    break;
                }
                case command::type::remove: {
                    break;
                }
                case command::type::execute: {
                    break;
                }
                case command::type::snapshot: {
                    break;
                }
                default: {
                    log->warn("unknown input cc process");
                }
            }
            return {};
        }

        /**
         * @brief run next scheduled dag
         * @return time of the next scheduled dag.
         */
        auto next_scheduled_dag() const noexcept -> std::optional<notification_type> {
            std::lock_guard lock(this->mutex);

            return {
                notification_type {
                    .time = std::chrono::steady_clock::now() + std::chrono::seconds{5},
                    .associated_request = {},
                    .command_type_ = command::type::execute
                }
            };
        }


    private:

        inline bool unique_name (cosmos::shyguy_request const& r) const {
            return not this->dags.contains(r.name);
        }

        std::unordered_map<std::string, directed_acyclic_graph> dags{};
        std::unordered_map<std::string, std::string> schedules{};
        mutable std::mutex mutex{};
        std::shared_ptr<spdlog::logger> log;
    };

    class notify_updater
    {
    public:

        auto notify_wakeup(notification_type notification) noexcept -> void {
            std::lock_guard lock(this->mutex_);
            cached_time_ = notification;
            this->condition_variable_.notify_one();
        }

        auto sleep_until_or_notified(std::chrono::steady_clock::time_point next_time) noexcept {
            std::unique_lock lock {this->mutex_};
            this->condition_variable_.wait_until(lock, next_time);
            auto result_time = cached_time_;
            cached_time_.reset();
            return result_time;
        }

    private:
        std::optional<notification_type> cached_time_{};
        mutable std::mutex mutex_{};
        std::condition_variable condition_variable_{};
    };

    inline auto function_schedule_runner (notify_updater& notifier, concurrent_shyguy& shy_guy) {
        // default sleep time
        auto sleep_until_dag_scheduled = [&] mutable -> notification_type
        {
            auto default_wait_time = std::chrono::steady_clock::now() + std::chrono::months(1);
            while(true)
            {
                if (auto notification  = notifier.sleep_until_or_notified(default_wait_time); notification.has_value())
                {
                    return notification.value();
                }
                else
                {
                    default_wait_time += std::chrono::months(1);
                }
            }
        };

        auto launch_dag_and_sleep_till_next = [&] (auto& notified) mutable
        {
            while (true)
            {
                if (auto notification = notifier.sleep_until_or_notified(notified.time); notification.has_value())
                {
                    notified = notification.value();
                    continue;
                }

                shy_guy.process(command::type::execute, notified.associated_request);

                auto const next_scheduled_time = shy_guy.next_scheduled_dag();
                if (not next_scheduled_time.has_value())
                    break;

                notified = next_scheduled_time.value();
            }
        };

        while (true)
        {
            auto notification = sleep_until_dag_scheduled();
            launch_dag_and_sleep_till_next(notification);
        }
    }

    inline auto super_run () -> bool {
        return false;
        /*
        auto file_logger    = spdlog::basic_logger_mt("file_log", "logs/file-log.txt", true);
        auto task_queue     = std::make_shared<blocking_queue<std::vector<task>>>();
        //  auto io_queue   = std::make_shared<blocking_queue<std::vector<task>>>();
        auto work_pool      = exec::static_thread_pool{3};
        auto work_scheduler = work_pool.get_scheduler();
        auto scope          = exec::async_scope{};
        //1. auto dependency_map = concurrent_map<string, dags>{}
        //2. auto task_maintainer = concurrent_task_maintainer{};

        ex::sender auto worker = work_sender(work_scheduler, file_logger, task_queue);
        scope.spawn(std::move(worker));

        scope.spawn(
            ex::on(work_scheduler,
                ex::just(poll_parameters{file_logger, task_queue})
                | ex::then(polling_work)
            )
        );

        scope.spawn(
             ex::on(work_scheduler,
                ex::just()
                | ex::then([] {
                    while (false) {
                        //auto value = co_await zmq();
                        //io_queue.enqueue(co_await zmq());
                    }
                })
            )
        );

        (void) stdexec::sync_wait(scope.on_empty());
        work_pool.request_stop();*/
    }
} // namespace cosmos::inline v1
