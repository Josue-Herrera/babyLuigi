#pragma once

#include <filesystem>
#include <varaint>
#include <chrono>

#include "task.hpp"
#include "task_system.hpp"
#include "graph/graph.hpp"

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <nlohmann/json.hpp>

namespace jx 
{
    inline namespace v1
    {
        using json = nlohmann::json;
        enum class respose_reason { not_set, revoking, completing };

        
        class graph_runner_type
        {
        public: 
            using id_type        = std::string; // converted from a uint16_t
            using graph_type     = directed_acyclic_graph;
            using directory_type = std::filesystem::path;
        
        private:
            class implementation_type {
            public:

                auto run(zmq::context_t& context) noexcept -> void {

                    using namespace std::chrono_literals;
                    
                    zmq::socket_t task_subscriber(context, zmq::socket_type::sub);
                    task_subscriber.set(zmq::sockopt::subscribe, id_);
                    zmq::poller_t<> subscriber_poller{};
                    subscriber_poller.add(task_subscriber, zmq::event_flags::pollin);
                    std::vector<zmq::poller_event<>> events(1);
                    

                    auto receive_task = [&](auto& pack) mutable noexcept {
                        try {
                            return zmq::recv_multipart(task_subscriber, pack.data());
                        }
                        catch (std::runtime_error const& error) {
                            spdlog::error("failed to recv message : {} ", error.what());
                            return std::optional<size_t>{};
                        }
                    };

                    while (true) {

                        std::array message_pack{ zmq::message_t(6), zmq::message_t() };

                        if (auto received = subscriber_poller.wait_all(events, 100ms); received)
                        {
                            if (auto const size = receive_task(message_pack); not size)
                                continue;

                            auto const binary_request = message_pack[1].to_string_view();
                            auto const unpacked_json  = json::from_bson(binary_request);
                            auto const task           = unpacked_json.template get<jx::task>();

                            if (auto const error = graph_.push_task(task); check(error)) {
                                spdlog::error("task {} failed because {} ", task.name, to_cstring(error));
                                continue;
                            }


                            



                            
                        }
                        events.front() = zmq::poller_event<>{};
                    }
                }

                friend auto operator<=>(implementation_type const& lhs, implementation_type const& rhs) {
                    return std::tuple {lhs.id_} <=> std::tuple {rhs.id_};
                }
                
                
                id_type id_;
                std::string root_name_;
                graph_type graph_;
                directory_type directory_;
                task root_task_;

            } impl_;
        
        public:

            graph_runner_type(zmq::context_t& context, id_type id, std::string&& name, graph_type&& graph, directory_type&& directory, task&& root_task) noexcept
                : impl_ {
                    id, 
                    std::forward<std::string>(name), 
                    std::forward<graph_type>(graph),
                    std::forward<directory_type>(directory),
                    std::forward<task>(root_task)
                }
            {
                runner_ = std::thread{ [&]() mutable { impl_.run(context); } };
            }

            friend auto operator<=>(graph_runner_type const& lhs, graph_runner_type const& rhs) {
                return std::tuple {lhs.impl_.id_}<=> std::tuple {rhs.impl_.id_};
            }
            
        private:    
            std::thread runner_;
        };

        class task_maintainer_type {
        public:
        
        
        private:




            std::vector<graph_runner_type> task_runners{};
        };


        class schedule {
            enum class occurrence { monthly, weekly, daily, hourly };
            

        };


    } // namespace v1
} // namespace jx
