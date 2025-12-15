#pragma once

// *** Project Includes ***
#include "graph/graph.hpp"
#include "shyguy_request.hpp"
#include "fwd_vocabulary.hpp"
#include "fs_storage.hpp"

// *** 3rd Party Includes ***
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>

namespace cosmos::inline v1
{
    class concurrent_shyguy
    {
        using root_name_str = std::string;
        using name_str      = std::string;
        using cron_tab_str  = std::string;
        using logger_t      = std::shared_ptr<spdlog::logger>;
    public:
        explicit concurrent_shyguy(request_queue_t rq, terminator_t t, data_storage storage):
            logger{spdlog::get("shyguy_logger")}, request_queue{std::move(rq)}, running{std::move(t)}, storage{std::move(storage)}
         {}


        /**
         * @brief processes dags and tasks from baby luigi client.
         *
         * @param type the enum that determines what type it is.
         * @param request the request holding the data.
         */
        auto process(cosmos::shyguy_request const &request) noexcept -> command_result_type;

        auto create(shyguy_dag const &dag) noexcept -> command_result_type;
        auto remove(shyguy_dag const &dag) noexcept -> command_result_type;
        auto execute(shyguy_dag const &dag) noexcept -> command_result_type;
        auto execute_at(shyguy_dag const& dag, std::chrono::steady_clock::time_point scheduled_time) noexcept -> command_result_type;
        auto snapshot(shyguy_dag const &dag) noexcept -> command_result_type;

        auto create(shyguy_task const &task) noexcept -> command_result_type;
        auto remove(shyguy_task const &task) noexcept -> command_result_type;
        auto execute(shyguy_task const &task) noexcept -> command_result_type;
        auto snapshot(shyguy_task const &task) noexcept -> command_result_type;

        auto process(std::monostate) const noexcept -> command_result_type;
        auto next_scheduled_dag() const noexcept -> std::optional<notification_type>;
        [[nodiscard]] auto storage_handle() const noexcept -> data_storage { return storage; }

    private:
        bool has_unique_name(shyguy_dag const &request) const { return not dags.contains(request.name); }
        bool has_unique_name(shyguy_task const &request) const
        {
            if (const auto iter = dags.find(request.associated_dag); iter != dags.end())
                return not iter->second.contains(request.name);
            return false;
        }

        [[nodiscard]] auto make_task_metadata(shyguy_task const &task) const -> std::optional<task_metadata>;

        template<class... T>
        auto log_return(fmt::format_string<T...> fmt_str, T&&... args) noexcept -> std::string
        {
            auto value = fmt::format(fmt_str, args...);
            logger->info(value);
            return value;
        }

        std::unordered_map<name_str, shyguy_task> task_map{};
        std::unordered_map<root_name_str, directed_acyclic_graph> dags{};
        std::unordered_map<root_name_str, cron_tab_str> schedules{};
        std::vector<root_name_str> running_dags{};
        std::vector<name_str> running_tasks{};

        mutable std::recursive_mutex mutex{};
        std::shared_ptr<spdlog::logger> logger;
        request_queue_t request_queue;
        terminator_t    running;
        data_storage storage{};
        std::atomic_uint64_t task_request_sequence{0};
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

    template <class AfterExecute>
    inline auto function_schedule_runner(notify_updater &notifier,
                                         concurrent_shyguy &shy_guy,
                                         terminator_t running,
                                         AfterExecute after_execute)
    {
        // default sleep time
        auto sleep_until_dag_scheduled = [&]() mutable -> notification_type
        {
            auto default_wait_time = std::chrono::steady_clock::now() + std::chrono::months(1);
            while (running->load(std::memory_order_relaxed))
            {
                if (auto notification = notifier.sleep_until_or_notified(default_wait_time); notification.has_value())
                {
                    return notification.value();
                }

                default_wait_time += std::chrono::months(1);
            }

            return {.time = std::chrono::steady_clock::now(), .associated_request = {}, .command_type = command_enum::error};
        };

        auto launch_dag_and_sleep_till_next = [&](auto &notified) mutable
        {
            while (running->load(std::memory_order_relaxed))
            {
                if (auto notification = notifier.sleep_until_or_notified(notified.time); notification.has_value())
                {
                    notified = notification.value();
                    continue;
                }

                notified.associated_request.command = command_enum::execute;
                (void) shy_guy.process(notified.associated_request);
                after_execute();

                auto const next_scheduled_time = shy_guy.next_scheduled_dag();
                if (not next_scheduled_time.has_value())
                    break;

                notified = next_scheduled_time.value();
            }
        };

        while (running->load(std::memory_order_relaxed))
        {
            auto notification = sleep_until_dag_scheduled();
            if (not running->load(std::memory_order_relaxed))
                break;
            launch_dag_and_sleep_till_next(notification);
        }
    }

    inline auto function_schedule_runner(notify_updater &notifier, concurrent_shyguy &shy_guy, terminator_t running)
    {
        function_schedule_runner(notifier, shy_guy, std::move(running), [] {});
    }

} // namespace cosmos::inline v1
