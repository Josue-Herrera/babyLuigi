// Project Inc
#include <cron_expression.hpp>
#include <process/file_write.hpp>
#include <process/system_execution.hpp>
#include <shyguy_request.hpp>
#include "concurrent_shyguy.hpp"
#include "interactive_shyguy.hpp"
#include "shyguy.hpp"

// 3rdparty Inc
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <nlohmann/json.hpp>
#include <range/v3/all.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexec/execution.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>

// Standard Inc
#include <string>
#include <unordered_map>

namespace cosmos::inline v1 {
		using namespace std::string_literals;
		using json = nlohmann::json;
        /*
		void shyguy::process_json(json const& json_req) {
			// this is a function
			auto request = json_req.template get<cosmos::shyguy_request>();
			spdlog::info("Name String {} ", request.name);

			// this is a function
			if (not write_to_file(request)) {
				spdlog::warn("failed to write file: {}", request.filename.value());
			}

			// this is a function
			std::string command = "python3 " + request.filename.value();
			auto aaaa = cosmos::execute(command);
			spdlog::info("executing {} output {}", request.filename.value(), aaaa.output);
		}


        auto zmq_stuff() {
		    // read from config file
		    enum index { identity, data };
		    enum errors { no_error, bad_message, try_again };

		    zmq::context_t context(1);
		    zmq::socket_t responder(context, zmq::socket_type::router);

		    //responder.bind("tcp://127.0.0.1:" + arguments_.port);

		    auto receive_task = [&] (auto& pack) mutable noexcept {
		        try {
		            return zmq::recv_multipart(responder, pack.data());
		        }
		        catch (std::runtime_error const& error) {
		            spdlog::error("failed to recv message : {} ", error.what());
		            return std::optional<size_t>{};
		        }
		    };

		    // auto reply_to_task = [&](auto& pack) mutable noexcept {
		    // 	try {
		    // 		return zmq::send_multipart(responder, pack);
		    // 	}
		    // 	catch (std::runtime_error const& error) {
		    // 		spdlog::error("failed to recv message : {} ", error.what());
		    // 		return std::optional<size_t>{};
		    // 	}
		    // };

		    while (true)
		    {
		        std::array message_pack { zmq::message_t(5), zmq::message_t(100) };

		        if (auto const size = receive_task(message_pack); size)
		        {
		            auto const request_identity = message_pack[identity].to_string_view();
		            auto const binary_request = message_pack[data].to_string_view();
		            auto const unpacked_json  = json::from_bson(binary_request);
		            auto const task           = unpacked_json.template get<cosmos::shyguy_request>();
		            spdlog::info("request identity {}", request_identity.size(), task.name);
		            zmq::send_multipart(responder, message_pack);
		        }
		    }
		}*/

		auto shyguy::run() noexcept -> void {
		    auto file_logger = spdlog::basic_logger_mt("file_log", "logs/file-log.txt", true);
		    auto shyguy      = concurrent_shyguy{file_logger};

		    // auto request_queue  = std::make_shared<blocking_queue<std::vector<shyguy_request>>>();
		    // auto io_queue       = std::make_shared<blocking_queue<std::vector<task>>>();
		    auto high_level_pool      = exec::static_thread_pool{3};
		    auto high_level_scheduler = high_level_pool.get_scheduler();
		    auto scope                = exec::async_scope{};

		    auto interactive_view =
                stdexec::starts_on(high_level_scheduler,
                    stdexec::just(arguments.interactive)
                    | stdexec::then([&] (bool interactive) {
                        file_logger->info("{}", interactive);
                        if (interactive) {
                            interactive_shyguy i{shyguy};
                            i.demo();
                        }
                    })
                );

		    scope.spawn(std::move(interactive_view));

		    (void) stdexec::sync_wait(scope.on_empty());
		    high_level_pool.request_stop();

		};
	}

