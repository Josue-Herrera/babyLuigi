//
// Created by josue h. on 9/15/2024.
//
#pragma once
#ifndef ZMQ_CONSUMER_HPP
#define ZMQ_CONSUMER_HPP

#include "blocking_queue.hpp"
#include <zmq.hpp>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include <map>
#include <list>


namespace cosmos {

    namespace zmq_constants {
         static constexpr const std::size_t max_identity_size = 5;
         enum index : std::size_t { identity, payload };
         enum response_status : std::size_t { not_set, success, error };
    }

   inline auto receive_task (zmq::socket_ref responder, auto& pack) noexcept -> std::optional<std::size_t> {
        try {
            return { zmq::recv_multipart(responder,  std::data(pack)) };
        }
        catch (std::runtime_error const& error) {
            spdlog::error("failed to recv message : {} ", error.what());
            return { std::nullopt };
        }
    };

    inline auto reply_to_task(zmq::socket_ref responder, auto& pack) noexcept -> std::optional<std::size_t>{
        try {
            return zmq::send_multipart(responder, pack);
        }
        catch (std::runtime_error const& error) {
            spdlog::error("failed to recv message : {} ", error.what());
            return std::optional<size_t>{};
        }
    };

    struct response_type { zmq_constants::response_status status{}; std::array<zmq::message_t, 2> message; };
    using zmq_response_type = std::optional<response_type>;
    using  zmq_callback     = auto (*) (zmq_constants::response_status&, task) -> void;
    struct zmq_consumer_params {
        int port {6969};
        zmq_callback callback {};
        blocking_queue<zmq_response_type>&  results;
    };

    inline auto  response_to_msg (zmq_response_type const&) -> zmq::message_t { return {}; }

    inline auto zmq_consumer(const zmq_consumer_params parameters)
    {
        using zmq_constants::index;

        zmq::context_t context(1);
        zmq::socket_t responder(context, zmq::socket_type::router);
        responder.bind("tcp://127.0.0.1:" + std::to_string(parameters.port));

        std::list<zmq_response_type> responses{};
        std::array message_pack { zmq::message_t(5), zmq::message_t(100) };

        exec::async_scope scope;
        exec::static_thread_pool pool{16};
        stdexec::scheduler auto scheduler = pool.get_scheduler();

        while (true) {
            if (auto const size = receive_task(responder, message_pack); size) {
                auto const binary_request = message_pack[index::payload].to_string_view();
                auto const unpacked_json  = nlohmann::json::from_bson(binary_request);
                responses.emplace_front();

                auto handle_command =
                    stdexec::starts_on(scheduler,
                        stdexec::just(unpacked_json.get<task>())
                        | stdexec::then(
                            [&, response = responses.begin()](task const& command) mutable {
                                auto& value = response->value().status;
                               parameters.callback(value, command);
                            }
                        )
                    );

                scope.spawn(std::move(handle_command));
            }

            std::erase_if(responses, [] (const auto&) {
                //if (response)
                   // responder.send(response.value().message);
                return false;
            });
        }

        stdexec::sync_wait(scope.on_empty());
        pool.request_stop();
    }
}
#endif //ZMQ_CONSUMER_HPP
