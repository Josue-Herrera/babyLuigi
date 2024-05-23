#pragma once

#include "task_system.hpp"
#include "task_queue.hpp"
#include "graph/graph.hpp"

namespace jx 
{
    inline namespace v1
    {   
        enum class respose_reason { not_set, revoking, completing };

        struct task_view {
            std::string_view request{};

            respose_reason response{};

            std::shared_ptr<blocking_queue<respose_reason>> response_queue{};
        };
  
        class task_maintainer : public task_system {
        public:
            using task_map = std::unordered_map<std::string, std::vector<std::string>>;

            [[nodiscard]] auto service(task_view) noexcept -> bool;
        

        private:
        
            class task {


            };     

            task_map root_to_dependencies{};


        };


    } // namespace v1
} // namespace jx
