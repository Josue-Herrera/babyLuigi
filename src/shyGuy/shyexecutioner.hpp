#pragma once

// *** Project Includes ***
#include "fwd_vocabulary.hpp"

// *** Standard Includes ***
#include <memory>
#include <vector>
#include <atomic>

namespace cosmos::inline v1
{
    class shy_executioner
    {
        explicit shy_executioner(terminator_t r, request_queue_t rq) noexcept
            : running{std::move(r)}, request_queue{std::move(rq)} {}

        auto dag_processing_factory_line() const noexcept -> void;
        auto dispatcher() noexcept -> void;

        terminator_t    running;
        request_queue_t request_queue;
    };

} // namespace cosmos::inline v1

