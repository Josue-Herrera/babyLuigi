
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
#include <CLI/CLI.hpp>
#include <fmt/format.h>

namespace cosmos
{
    inline namespace v1
    {
        using json = nlohmann::json;


        auto create_request(shyguy_request const& task) noexcept {

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

        auto submit_task_request(shyguy_request const& task,  shy_guy_info const& info) noexcept -> bool {
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
                auto const request         = unpacked_json.template get<cosmos::shyguy_request>();
                spdlog::info("Received reply [ size={} msg={} ]", message_pack.size(),  request.name);
            }

            return true;
        }

        auto valid(shyguy_request const& request) noexcept -> bool
        {
            if (auto const* task = std::get_if<shyguy_task>(&request.data))
            {
                if (task->filename)
                {
                    std::filesystem::path file_path(*task->filename);
                    if (not std::filesystem::exists(file_path))
                    {
                        spdlog::error("File does not exist: {}", *task->filename);
                        return false;
                    }

                    if (not std::filesystem::is_regular_file(file_path))
                    {
                        spdlog::error("This is not a regular file: {}", *task->filename);
                        return false;
                    }
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
} // namespace cosmos

namespace cosmos
{
    using namespace cosmos::v1;

    static void attach_baby_cli(CLI::App& app, baby_cli_state& state)
    {
        add_babyluigi_subcommands(app, state);
    }

    auto run_cli_and_submit(int argc, const char** argv) -> int
    {
        CLI::App app{fmt::format("{}\nversion {}", cosmos::baby_luigi_banner, "0.0.0")};
        app.require_subcommand(1);

        baby_cli_state state{};
        attach_baby_cli(app, state);

        try {
            CLI11_PARSE(app, argc, argv);
        } catch (const CLI::ParseError& e) {
            return app.exit(e);
        }

        // If task has a file, populate contents
        if (auto* task = std::get_if<shyguy_task>(&state.request.data))
        {
            if (task->filename)
            {
                if (!cosmos::v1::valid(state.request))
                    return EXIT_FAILURE;

                task->file_content = cosmos::v1::download_contents(task->filename.value());
            }
        }

        if (!cosmos::v1::submit_task_request(state.request, state.info))
            return EXIT_FAILURE;

        return EXIT_SUCCESS;
    }
}
