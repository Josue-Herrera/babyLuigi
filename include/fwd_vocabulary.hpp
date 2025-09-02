#pragma once

// *** Standard Includes ***
#include <atomic>
#include <memory>
#include <vector>

namespace cosmos::inline v1
{
    class directed_acyclic_graph;
    class task_runner;
    class shyguy_request;
    template <typename T> class blocking_queue;

    using terminator_t     = std::shared_ptr<std::atomic_bool>;
    using task_request     = std::pair<std::vector<task_runner>, directed_acyclic_graph>;
    using request_queue_t  = std::shared_ptr<blocking_queue<task_request>>;
    using response_queue_t = std::shared_ptr<blocking_queue<shyguy_request>>;

} // namespace cosmos::inline v1