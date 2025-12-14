//
// Created by josue h. on 9/15/2024
//
#pragma once

// *** Project Includes ***
#include "blocking_queue.hpp"
#include "shyguy_request.hpp"
#include "fwd_vocabulary.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include <stdexec/execution.hpp>
#include <spdlog/spdlog.h>
#include <zmq_addon.hpp>

// *** Standard Includes ***
#include <list>
#include <map>

namespace cosmos::inline v1
{
    namespace zmq_constants
    {
        static constexpr const std::size_t max_identity_size = 5; // 5 bytes for the identity string array
        static constexpr const std::size_t max_payload_size = 1024 * 1024; // 1 MB max payload size

        enum index : std::size_t
        {
            identity,
            payload
        };

        enum response_status : std::size_t
        {
            not_set,
            success,
            error
        };

    } // namespace zmq_constants

    inline auto receive_task(zmq::socket_ref responder, auto &pack) noexcept -> std::optional<std::size_t>
    {
        try
        {
            return zmq::recv_multipart(responder, std::data(pack));
        }
        catch (std::runtime_error const &error)
        {
            spdlog::error("failed to recv message : {} ", error.what());
            return {std::nullopt};
        }
    };

    inline auto reply_to_task(zmq::socket_ref responder, auto &pack) noexcept -> std::optional<std::size_t>
    {
        try
        {
            return zmq::send_multipart(responder, pack);
        }
        catch (std::runtime_error const &error)
        {
            spdlog::error("failed to recv message : {} ", error.what());
            return std::optional<size_t>{};
        }
    };

    struct response_type
    {
        zmq_constants::response_status status{};
        std::array<zmq::message_t, 2> message;
    };
    using zmq_response_type = std::optional<response_type>;

    struct zmq_consumer_params
    {
        int port{6969};
        blocking_queue<zmq_response_type> &results;
    };

    inline auto response_to_msg(zmq_response_type const &) -> zmq::message_t { return {}; }

    /*
    inline auto zmq_consumer(const zmq_consumer_params parameters)
    {
        using zmq_constants::index;

        zmq::context_t context(1);
        zmq::socket_t responder(context, zmq::socket_type::router);
        responder.bind("tcp://127.0.0.1:" + std::to_string(parameters.port));

        std::list<zmq_response_type> responses{};
        std::array message_pack{zmq::message_t(5), zmq::message_t(100)};


        while (true)
        {
            if (auto const size = receive_task(responder, message_pack); size)
            {
                auto const binary_request = message_pack[index::payload].to_string_view();
                auto const unpacked_json = nlohmann::json::from_bson(binary_request);
                responses.emplace_front();

                auto handle_command = 
                stdexec::just(unpacked_json.get<shyguy_request>()) |
                stdexec::then(
                    [&, response = responses.begin()](shyguy_request const &command) mutable
                    {
                        auto &value = response->value().status;
            
                    });

                scope.spawn(stdexec::starts_on(scheduler, std::move(handle_command)));
            }

            std::erase_if(responses,
                          [](const auto &)
                          {
                              // if (response)
                              //  responder.send(response.value().message);
                              return false;
                          });
        }

        stdexec::sync_wait(scope.on_empty());
        pool.request_stop();
    }
    */
    class zmq_router
    {
    public:
        explicit zmq_router(request_queue_t rq, terminator_t r, int32_t port = 6969) noexcept
            : running{std::move(r)}, request_queue{std::move(rq)} 
            {
                responder.bind("tcp://127.0.0.1:" + std::to_string(port));
            }

       public:  auto route_data() ->  std::optional<shyguy_request> ;

    private:

        auto reply_to_task(auto& pack) noexcept -> std::optional<std::size_t>;


        
        terminator_t    running;
        request_queue_t request_queue;
        zmq::context_t  context{1};
        zmq::socket_t   responder{context, zmq::socket_type::router};
        std::list<zmq_response_type> responses{};
    };
    
} // namespace cosmos::inline v1
