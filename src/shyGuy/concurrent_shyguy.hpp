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

    enum class command_enum : uint8_t {
        create,
        remove,
        execute,
        snapshot,
        command_error
    };

    struct command_result_type {
        enum options : uint8_t {
            successful,
            command_error
        };
        std::string message{};
        options result{};
    };
    struct notification_type {
        std::chrono::steady_clock::time_point time;
        cosmos::shyguy_request associated_request;
        command_enum command_type;
    };
    struct task_runner {
        cosmos::shyguy_request request;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        command_result_type result;
        std::size_t index{};
    };

    class concurrent_shyguy {
    public:
        explicit concurrent_shyguy (const std::shared_ptr<spdlog::logger> &l):
        log{l} {

        }

        /**
         * @brief Create DAG in a concurrent manner
         *
         * @param input
         * @param request
         */
        auto process(command_enum const input, cosmos::shyguy_request const &request) noexcept -> command_result_type {
            std::lock_guard lock(mutex);

            request.data | match {
                [&, input](shyguy_dag const& dag) mutable {
                    switch (input) {
                        case command_enum::create: {
                            // check if dag already exists
                            auto dag_iter = dags.find(dag.name);
                            if (dag_iter == dags.end()) {
                                dags.emplace(dag.name, directed_acyclic_graph(dag.name));

                                if (has_schedule(dag))
                                    schedules.emplace(dag.name, dag.schedule.value());

                                log->info("create dag {}",dag.name);
                            }
                            else {
                                log->info("dag already exists {}", dag.name);
                            }
                        }
                        case command_enum::remove: {
                            if (bool const erased = dags.erase(dag.name); erased) {
                                if (has_schedule(dag))
                                    schedules.erase(dag.name);

                                log->info("removed dag {}",dag.name);
                            }
                            else {
                                log->info("dag failed to remove {}", dag.name);
                            }

                            break;
                        }
                        case command_enum::execute: {
                            // find dag, check if its running
                            if (const auto dag_iter = dags.find(dag.name); dag_iter == dags.end()) {
                                log->info("dag does not exist, cannot run {}",dag.name);
                                break;
                            }

                            log->info("attempting to execute dag {}", dag.name);

                            //


                            break;
                        }
                        case command_enum::snapshot: {
                            break;
                        }
                        default: {
                            // log->info("Command Not Supported: {} ", to_string(input));
                        }
                    }


                }, 
                [input](shyguy_task const& task) {

                },
                [&](std::monostate) { log->info("Monostate input. This should never happen.");},
                [&,input](auto&& args) {
                   log->info("idk {} {}",std::to_underlying(input), args.name);
                }
            };

            return {};
        };


        auto nonlocking_dag_process(command_enum const input, shyguy_dag const& dag) noexcept {
            switch (input) {
                case command_enum::create: {
                    dags.emplace(dag.name, directed_acyclic_graph(dag.name));

                    if (has_schedule(dag))
                        dags.emplace(dag.name, dag.schedule.value());

                    log->info("create dag {}",dag.name);
                }
                case command_enum::remove: {
                    break;
                }
                case command_enum::execute: {
                    break;
                }
                case command_enum::snapshot: {
                    break;
                }
                default: {
                    // log->info("Command Not Supported: {} ", to_string(input));
                }
            }
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
                    .command_type = command_enum::execute
                }
            };
        }


    private:

        inline bool has_unique_name (cosmos::shyguy_dag const& request) const {
            return not dags.contains(request.name);
        }

        inline bool has_unique_name (cosmos::shyguy_task const& request) const {

            if (auto iter = dags.find(request.associated_dag); iter != dags.end())
                return iter->second.contains(request.name);

            return false;
        }

        std::unordered_map<std::string, directed_acyclic_graph> dags{};
        std::unordered_map<std::string, std::string> schedules{};
        std::vector<std::string> running_dags{};
        std::vector<std::string> running_tasks{};

        mutable std::mutex mutex{};
        std::shared_ptr<spdlog::logger> log;
    };

    class notify_updater
    {
    public:

        auto notify_wakeup(const notification_type& notification) noexcept -> void {
            std::lock_guard lock(this->mutex_);
            cached_time_ = notification;
            this->condition_variable_.notify_one();
        }

        auto sleep_until_or_notified(const std::chrono::steady_clock::time_point next_time) noexcept {
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
        auto sleep_until_dag_scheduled = [&]() mutable -> notification_type
        {
            auto default_wait_time = std::chrono::steady_clock::now() + std::chrono::months(1);
            while(true)
            {
                if (auto notification  = notifier.sleep_until_or_notified(default_wait_time); notification.has_value())
                {
                    return notification.value();
                }

                default_wait_time += std::chrono::months(1);
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

                shy_guy.process(command_enum::execute, notified.associated_request);

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
