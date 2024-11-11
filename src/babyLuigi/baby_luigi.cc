
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
#include <random>

namespace cosmos
{
    inline namespace v1
    {
        using json = nlohmann::json;


        auto create_request(task const& task) noexcept {

            return  zmq::message_t { json::to_bson(task) };
        }

        std::string create_random_router_id() {
            constexpr std::array characters = {'a','b','c', 'd','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s',
                't','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W',
                'X','Y','Z','0','1','2','3','4','5','6','7','8','9'};

            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<size_t> distribution(0, std::ssize(characters) - 1);

            return {
                characters[distribution(generator)],
                characters[distribution(generator)],
                characters[distribution(generator)],
                characters[distribution(generator)],
                characters[distribution(generator)]
            };
        }

        auto submit_task_request(task const& task,  shy_guy_info const& info) noexcept -> bool {
            zmq::context_t context(1);

            zmq::socket_t requester(context, zmq::socket_type::dealer);
            requester.connect(std::string("tcp://") + info.server_address + ":" + info.port);
            requester.set(zmq::sockopt::routing_id, create_random_router_id());
            requester.send(create_request(task), zmq::send_flags::none);
            zmq::message_t test{ };
            std::array message_pack {zmq::message_t{}, zmq::message_t{}};

            if (auto result = zmq::recv_multipart(requester, message_pack.data()); result) {
                auto const binary_request = message_pack[1].to_string_view();
                auto const unpacked_json  = json::from_bson(binary_request);
                auto const request         = unpacked_json.template get<cosmos::task>();
                spdlog::info("Received reply [ size={} msg={} ]", message_pack.size(),  request.name);
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
                return {std::istreambuf_iterator<char>{source_file}, {}};
            }

            spdlog::error("Unable to correctly open file: {} .", path);
            return {};
        }
    } // namespace v1
} // namespace jx
