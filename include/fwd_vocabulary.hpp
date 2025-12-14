#pragma once

// *** Standard Includes ***
#include <atomic>
#include <memory>
#include <optional>
#include <ranges>
#include <vector>


namespace cosmos::inline v1
{
    class directed_acyclic_graph;
    class task_runner;
    class shyguy_request;
    template <typename T> class blocking_queue;
    class fs_storage;
    class concurrent_shyguy;

    using terminator_t     = std::shared_ptr<std::atomic_bool>;
    using task_request     = std::pair<std::vector<task_runner>, directed_acyclic_graph>;
    using request_queue_t  = std::shared_ptr<blocking_queue<task_request>>;
    using input_queue_t    = std::shared_ptr<blocking_queue<shyguy_request>>;
    using data_storage     = std::shared_ptr<fs_storage>;
    using concurrent_shyguy_t = std::shared_ptr<concurrent_shyguy>;

    // Helper functions
    constexpr auto if_it_exists = [](auto const& entry)
    {
        return entry.has_value();
    };

    constexpr auto unwrap = [](auto const& entry) { return entry.value(); };

    constexpr auto unwrap_optionals = []
    {
        return std::views::filter(if_it_exists)
        | std::views::transform(unwrap);
    };
} // namespace cosmos::inline v1