 #pragma once

// *** Project Includes ***
#include "fwd_vocabulary.hpp"

// *** Standard Includes ***
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>

namespace cosmos::inline v1
{
    class shy_executioner
    {
    public:
        explicit shy_executioner(terminator_t r, request_queue_t rq) noexcept
            : running{std::move(r)}, request_queue{std::move(rq)} {}

        auto run_until_idle(std::chrono::milliseconds idle_timeout,
                            std::size_t max_dag_concurrency,
                            std::size_t max_task_concurrency) noexcept -> void;

    private:
        terminator_t    running;
        request_queue_t request_queue;
    };

} // namespace cosmos::inline v1

