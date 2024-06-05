#pragma once

#include <filesystem>

#include "task.hpp"
#include "task_system.hpp"
#include "graph/graph.hpp"
#include "zmq.hpp"

namespace jx 
{
    inline namespace v1
    {   
        enum class respose_reason { not_set, revoking, completing };

        
        class graph_runner
        {
            using id_type        = std::uint32_t;
            using graph_type     = directed_acyclic_graph;
            using directory_type = std::filesystem::path;
            
            class implementation_type {
            public:

                friend auto operator<=>(implementation_type const& lhs,implementation_type const& rhs) {
                    return lhs.id_ <=> rhs.id_;
                }

                id_type id_;
                std::string root_name_;
                graph_type graph_;
                directory_type directory_;
                task root_task_;
            } impl;


            graph_runner(id_type id, std::string name, graph_type graph, directory_type directory, task root_task) noexcept
                : impl{id, name, graph, directory, root_task }
            {


            }
            // graph runner { .id=(ZMQ_FILTER), .name=(root name?), .dag=(task_graph)  .directory=(directory), .tasks={task} };


            

            std::jthread runner_;
        };

        class task_maintainer_type {
        public:
        
        
        private:




            std::vector<graph_runner> task_runners{};
        };


    } // namespace v1
} // namespace jx
