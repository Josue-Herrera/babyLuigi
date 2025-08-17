#pragma once

// *** Standard Includes ***
#include <memory>
#include <vector>
#include <atomic>

namespace cosmos::inline v1
{
    class directed_acyclic_graph;
    class task_runner;
    template <typename T> class blocking_queue;

    class shy_executioner
    {
        using terminator_t = std::shared_ptr<std::atomic_bool>;
        using task_request    = std::pair<std::vector<task_runner>, directed_acyclic_graph>;
        using request_queue_t = std::shared_ptr<blocking_queue<task_request>>;

        explicit shy_executioner(terminator_t r, request_queue_t rq) noexcept
            : running{std::move(r)}, request_queue{std::move(rq)} {}

        auto dag_processing_factory_line() const noexcept -> void;
        auto dispatcher() noexcept -> void;

        std::shared_ptr<std::atomic_bool> running;
        request_queue_t request_queue;
    };

} // namespace cosmos::inline v1

