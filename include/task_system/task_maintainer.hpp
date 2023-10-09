#pragma once

#include "task_system.hpp"
#include "task_queue.hpp"

namespace jx 
{
    inline namespace v1
    {   
        enum class respose_reason { not_set, revoking, completing };

        struct task_view {
            std::string_view request{};

            respose_reason response{};

            std::shared_ptr<task_queue<respose_reason>> response_queue{};
        };
  
        class task_maintainer : task_system {
        public:
            auto service(task_view) noexcept -> bool;
        };

    } // namespace v1
} // namespace jx
