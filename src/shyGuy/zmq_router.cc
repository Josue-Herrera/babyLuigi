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

        std::array message_pack{zmq::message_t(5), zmq::message_t(100)};

        if (auto const size = receive_task(responder, message_pack); size)
        {
            auto const binary_request = message_pack[index::payload].to_string_view();
            auto const unpacked_json = nlohmann::json::from_bson(binary_request);
            return {unpacked_json.get<shyguy_request>()} ;
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