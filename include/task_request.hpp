//
// Created by babyLuigi on 12/15/2025.
//
#pragma once
#ifndef TASK_REQUEST_HPP
#define TASK_REQUEST_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "graph/graph.hpp"
#include "shyguy_request.hpp"

namespace cosmos::inline v1
{
    using task_request_payload = std::pair<std::vector<task_runner>, directed_acyclic_graph>;

    struct task_request
    {
        std::chrono::steady_clock::time_point scheduled_time{};
        std::uint64_t sequence{};
        task_request_payload payload{};
    };

    using task_request_ptr = std::shared_ptr<task_request>;

    struct task_request_ptr_compare
    {
        auto operator()(task_request_ptr const& lhs, task_request_ptr const& rhs) const noexcept -> bool
        {
            if (not lhs)
                return true;
            if (not rhs)
                return false;
            if (lhs->scheduled_time != rhs->scheduled_time)
                return lhs->scheduled_time > rhs->scheduled_time; // earlier time has higher priority
            return lhs->sequence > rhs->sequence; // earlier sequence has higher priority
        }
    };
}

#endif // TASK_REQUEST_HPP
