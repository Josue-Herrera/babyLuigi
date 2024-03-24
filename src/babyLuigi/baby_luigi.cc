

#include <baby_luigi.hpp>

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>
#include <filesystem>
#include <range/v3/all.hpp>


namespace jx
{
    inline namespace v1
    {
        using json = nlohmann::json;

        auto create_request(task_description const& task) noexcept -> zmq::message_t {

            return zmq::message_t { 
                json::to_bson(
                    json {
                        { "name", task.name },
                        { "type", task.type },
                        { "payload", task.payload }
                    }
                )   
            };
        }

        auto submit_task_request(task_description const& task,  shy_guy_info const& info) noexcept -> bool 
        {
            zmq::context_t context(1);

            zmq::socket_t requester(context, zmq::socket_type::req);
            requester.connect(info.server_address + ":" + info.port);

            requester.send(create_request(task), zmq::send_flags::none);

            zmq::message_t test{ };

            if (auto result = requester.recv(test); result)
                spdlog::info("Received reply [ size={} msg={} ]", *result,  test.to_string());

            return true;
        }

        auto valid(task_description const& task) noexcept -> bool
        {
            std::filesystem::path file_path(task.file_location);
            if (not std::filesystem::exists(file_path))
            {
                spdlog::error("File does not exist: {}", task.file_location);
                return false;
            }

            if(not std::filesystem::is_regular_file(file_path))
            {
                spdlog::error("This is not a regular file: {}", task.file_location);
                return false;
            }

            return true;
        }

        auto download_contents(std::string const& path) noexcept -> std::vector<char>
        {
            if (std::ifstream source_file { path, std::ios::binary }; source_file)
            {
                 auto a = std::vector<char>(std::istreambuf_iterator<char>{source_file}, {});

                 return a;
            }

            spdlog::error("Unable to correctly open file: {} .", path);

            return {};
        }
    } // namespace v1    
} // namespace jx
