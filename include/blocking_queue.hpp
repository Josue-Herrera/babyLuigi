//
// Created by joeue h. on 9/16/2024.
//
#pragma once
#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

namespace cosmos
{
    template <class T> struct blocking_queue
    {
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

    private:
        std::queue<T> q{};
        mutable std::mutex m{};
        std::condition_variable c{};
    };

} // namespace cosmos

#endif //BLOCKING_QUEUE_HPP
