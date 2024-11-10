
// *** Project Includes ***
#include "baby_luigi.hpp"

// *** 3rd Party Includes ***
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <fstream>
#include <filesystem>
#include <zmq_addon.hpp>

namespace cosmos
{
    inline namespace v1
    {
        using json = nlohmann::json;


        auto create_request(task const& task) noexcept {

            return  zmq::message_t { json::to_bson(task) };
        }

        auto submit_task_request(task const& task,  shy_guy_info const& info) noexcept -> bool {
            zmq::context_t context(1);

            zmq::socket_t requester(context, zmq::socket_type::dealer);
            requester.connect(std::string("tcp://") + info.server_address + ":" + info.port);
            requester.set(zmq::sockopt::routing_id, "hello");
            requester.send(create_request(task));
            zmq::message_t test{ };
            std::array message_pack {zmq::message_t{}, zmq::message_t{}};

            if (auto result = zmq::recv_multipart(requester, message_pack.data()); result) {
                auto const binary_request = message_pack[1].to_string_view();
                auto const unpacked_json  = json::from_bson(binary_request);
                auto const task           = unpacked_json.template get<cosmos::task>();
                spdlog::info("Received reply [ size={} msg={} ]", message_pack.size(),  task.name);
            }

            return true;
        }

        auto valid(task const& task) noexcept -> bool
        {
            if(task.filename)
            {
                std::filesystem::path file_path(*task.filename);
                if (not std::filesystem::exists(file_path))
                {
                    spdlog::error("File does not exist: {}", *task.filename);
                    return false;
                }

                if(not std::filesystem::is_regular_file(file_path))
                {
                    spdlog::error("This is not a regular file: {}", *task.filename);
                    return false;
                }
            }
            return true;
        }

        auto download_contents(std::string const& path) noexcept -> std::string
        {
            if (std::ifstream source_file { path, std::ios::binary }; source_file)
            {
                return std::string (std::istreambuf_iterator<char>{source_file}, {});
            }

            spdlog::error("Unable to correctly open file: {} .", path);
            return {};
        }
    } // namespace v1
} // namespace jx
