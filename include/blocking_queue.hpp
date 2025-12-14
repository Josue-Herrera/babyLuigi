//
// Created by joeue h. on 9/16/2024.
//
#pragma once
#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <chrono>
#include <condition_variable>
#include <optional>
#include <mutex>
#include <queue>

namespace cosmos::inline v1
{
    template <class T>
    class blocking_queue
    {
    public:
        constexpr auto enqueue(T&& t) noexcept -> void
        {
            std::lock_guard<std::mutex> lock(m);
            q.push(std::forward<T>(t));
            c.notify_one();
        }

        [[nodiscard]] constexpr auto dequeue() noexcept -> T
        {
            std::unique_lock<std::mutex> lock(m);
            while(q.empty()) c.wait(lock);
            T val = std::move(q.front());
            q.pop();
            return val;
        }

        template <class Rep, class Period>
        [[nodiscard]] constexpr auto dequeue_wait(const std::chrono::duration<Rep, Period>& timeout) noexcept -> std::optional<T>
        {
            std::unique_lock<std::mutex> lock(m);
            while(q.empty())
            {
                auto value = c.wait_for(lock, timeout);
                if (value == std::cv_status::timeout)
                {
                    return std::nullopt; // Timeout reached
                }
            }
            T val = std::move(q.front());
            q.pop();
            return { val };
        }

    private:
        std::queue<T> q{};
        mutable std::mutex m{};
        std::condition_variable c{};
    };

} // namespace cosmos

#endif //BLOCKING_QUEUE_HPP
