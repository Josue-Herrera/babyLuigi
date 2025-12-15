//
// Created by babyLuigi on 12/15/2025.
//
#pragma once
#ifndef BLOCKING_PRIORITY_QUEUE_HPP
#define BLOCKING_PRIORITY_QUEUE_HPP

#include <chrono>
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

namespace cosmos::inline v1
{
    namespace detail
    {
        template <class U>
        concept has_scheduled_time = requires(const U& u) { u.scheduled_time; } || requires(const U& u) { u->scheduled_time; };

        template <class U>
        [[nodiscard]] constexpr auto scheduled_time_of(const U& u) noexcept
            requires has_scheduled_time<U>
        {
            if constexpr (requires(const U& v) { v.scheduled_time; })
                return u.scheduled_time;
            else
                return u->scheduled_time;
        }
    } // namespace detail

    template <class T, class Compare = std::less<T>>
    class blocking_priority_queue
    {
    public:
        constexpr auto enqueue(T&& t) noexcept -> void
        {
            std::lock_guard<std::mutex> lock(mutex);
            heap.push_back(std::forward<T>(t));
            std::push_heap(heap.begin(), heap.end(), compare);
            condition.notify_one();
        }

        [[nodiscard]] constexpr auto dequeue() noexcept -> T
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (heap.empty())
                condition.wait(lock);
            std::pop_heap(heap.begin(), heap.end(), compare);
            auto val = std::move(heap.back());
            heap.pop_back();
            return std::move(val);
        }

        template <class Rep, class Period>
        [[nodiscard]] constexpr auto dequeue_wait(const std::chrono::duration<Rep, Period>& timeout) noexcept -> std::optional<T>
            requires detail::has_scheduled_time<T>
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            std::unique_lock<std::mutex> lock(mutex);
            while (true)
            {
                while (heap.empty())
                {
                    if (condition.wait_until(lock, deadline) == std::cv_status::timeout)
                        return std::nullopt;
                }

                auto const now = std::chrono::steady_clock::now();
                auto const& next_item = heap.front();
                if (detail::scheduled_time_of(next_item) <= now)
                {
                    std::pop_heap(heap.begin(), heap.end(), compare);
                    auto val = std::move(heap.back());
                    heap.pop_back();
                    return {std::move(val)};
                }

                auto const wake = std::min(deadline, detail::scheduled_time_of(next_item));
                if (condition.wait_until(lock, wake) == std::cv_status::timeout && wake == deadline)
                    return std::nullopt;
            }
        }

        template <class Rep, class Period>
        [[nodiscard]] constexpr auto dequeue_wait(const std::chrono::duration<Rep, Period>& timeout) noexcept -> std::optional<T>
            requires (!detail::has_scheduled_time<T>)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            std::unique_lock<std::mutex> lock(mutex);
            while (heap.empty())
            {
                if (condition.wait_until(lock, deadline) == std::cv_status::timeout)
                    return std::nullopt;
            }

            std::pop_heap(heap.begin(), heap.end(), compare);
            auto val = std::move(heap.back());
            heap.pop_back();
            return {std::move(val)};
        }

    private:
        std::vector<T> heap{};
        [[no_unique_address]] Compare compare{};
        mutable std::mutex mutex{};
        std::condition_variable condition{};
    };
} // namespace cosmos::inline v1

#endif // BLOCKING_PRIORITY_QUEUE_HPP
