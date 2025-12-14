// *** Project Includes ***
#include "zmq_router.hpp"

// *** 3rd Party Includes ***
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include <stdexec/execution.hpp>
#include <zmq.hpp>

// *** Standard Includes ***
#include <optional>
#include <list>
#include <map>

namespace cosmos::inline v1
{
    auto zmq_router::route_data() -> std::optional<shyguy_request> 
    {
        using namespace zmq_constants;

        // Receive identity + payload
        std::array<zmq::message_t, 2> message_pack{zmq::message_t{}, zmq::message_t{}};

        if (auto const size = receive_task(responder, message_pack); size)
        {
            try {
                auto const binary_request = message_pack[index::payload].to_string_view();
                auto const unpacked_json = nlohmann::json::from_bson(binary_request);
                auto request = unpacked_json.get<shyguy_request>();

                // Send ACK back to the dealer: identity envelope + "OK"
                static constexpr auto ok = std::string_view{"OK"};
                std::vector<zmq::const_buffer> reply{};
                reply.emplace_back(message_pack[index::identity].data(), message_pack[index::identity].size());
                reply.emplace_back(ok.data(), ok.size());
                (void) zmq::send_multipart(responder, reply);

                return {std::move(request)};
            } catch (std::exception const&) {
                // On parse failure, send ERROR ack
                static constexpr auto err = std::string_view{"ERROR"};
                std::vector<zmq::const_buffer> reply{};
                reply.emplace_back(message_pack[index::identity].data(), message_pack[index::identity].size());
                reply.emplace_back(err.data(), err.size());
                (void) zmq::send_multipart(responder, reply);
            }
        }

        return std::nullopt;
    }

    inline auto zmq_router::reply_to_task(auto &pack) noexcept -> std::optional<std::size_t>
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

} // namespace cosmos::inline v1
