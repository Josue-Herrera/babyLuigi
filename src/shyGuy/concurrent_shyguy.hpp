#pragma once

// *** Project Includes ***
#include "graph/graph.hpp"
#include "shyguy_request.hpp"
#include "blocking_queue.hpp"

// *** 3rd Party Includes ***
#include <spdlog/spdlog.h>
#include <zmq.hpp>

// *** Standard Includes ***
#include <filesystem>

namespace cosmos::inline v1
{
    class concurrent_shyguy
    {
        using root_name_str = std::string;
        using name_str      = std::string;
        using cron_tab_str  = std::string;
        using request_queue_t = std::shared_ptr<blocking_queue<shyguy_request>>;
        using logger_t = std::shared_ptr<spdlog::logger>;
    public:
        explicit concurrent_shyguy(request_queue_t rq):
            logger{spdlog::get("shyguy_logger")}, request_queue{std::move(rq)} {}

        /**
         * @brief processes dags and tasks from baby luigi client.
         *
         * @param type the enum that determines what type it is.
         * @param request the request holding the data.
         */
        auto process(command_enum type, cosmos::shyguy_request const &request) noexcept -> command_result_type;

        auto create(shyguy_dag const &dag) noexcept -> command_result_type;
        auto remove(shyguy_dag const &dag) noexcept -> command_result_type;
        auto execute(shyguy_dag const &dag) noexcept -> command_result_type;
        auto snapshot(shyguy_dag const &dag) noexcept -> command_result_type;

        auto create(shyguy_task const &task) noexcept -> command_result_type;
        auto remove(shyguy_task const &task) noexcept -> command_result_type;
        auto execute(shyguy_task const &task) noexcept -> command_result_type;
        auto snapshot(shyguy_task const &task) noexcept -> command_result_type;

        auto process(command_enum, std::monostate) const noexcept -> command_result_type;
        auto next_scheduled_dag() const noexcept -> std::optional<notification_type>;

    private:
        bool has_unique_name(shyguy_dag const &request) const { return not dags.contains(request.name); }
        bool has_unique_name(shyguy_task const &request) const
        {
            if (const auto iter = dags.find(request.associated_dag); iter != dags.end())
                return iter->second.contains(request.name);
            return false;
        }

        template<class... T>
        auto log_return(fmt::format_string<T...> fmt_str, T&&... args) noexcept -> std::string
        {
            auto value = fmt::format(fmt_str, args...);
            logger->info(value);
            return value;
        }

        std::unordered_map<root_name_str, directed_acyclic_graph> dags{};
        std::unordered_map<root_name_str, cron_tab_str> schedules{};
        std::vector<root_name_str> running_dags{};
        std::vector<name_str> running_tasks{};

        mutable std::mutex mutex{};
        std::shared_ptr<spdlog::logger> logger;
        request_queue_t request_queue;
    };

    class notify_updater
    {
    public:
        auto notify_wakeup(const notification_type &notification) noexcept -> void
        {
            std::lock_guard lock(this->mutex_);
            cached_time_ = notification;
            this->condition_variable_.notify_one();
        }

        auto sleep_until_or_notified(const std::chrono::steady_clock::time_point next_time) noexcept
        {
            std::unique_lock lock{this->mutex_};
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

    inline auto function_schedule_runner(notify_updater &notifier, concurrent_shyguy &shy_guy)
    {
        // default sleep time
        auto sleep_until_dag_scheduled = [&]() mutable -> notification_type
        {
            auto default_wait_time = std::chrono::steady_clock::now() + std::chrono::months(1);
            while (true)
            {
                if (auto notification = notifier.sleep_until_or_notified(default_wait_time); notification.has_value())
                {
                    return notification.value();
                }

                default_wait_time += std::chrono::months(1);
            }
        };

        auto launch_dag_and_sleep_till_next = [&](auto &notified) mutable
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

    inline auto super_run() -> bool
    {
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
