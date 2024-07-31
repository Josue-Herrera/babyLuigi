// Project Inc
#include <task.hpp>
#include "shy_guy.hpp"
#include <process/system_execution.hpp>
#include <process/file_write.hpp>

// 3rdparty Inc
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <range/v3/all.hpp>
#include <nlohmann/json.hpp>
#include <tl/expected.hpp>

// Standard Inc
#include <string>
#include <unordered_map>

namespace jx {
	inline namespace v1 {
		using namespace std::string_literals;		
		using json = nlohmann::json; 

		using directed_graphs_map = std::unordered_map<std::string, std::string>;
		using root_graphs_map = std::unordered_map<std::string, directed_graphs_map>;
		using root_dependency_map = std::unordered_map<std::string, std::vector<std::string>>;

		auto shy_guy::test_run() noexcept -> void{
			json mock_task1 = task { 
				.name = "root"s, 
				.type = "python"s, 
				.filename = "test.py"s,
				.file_content = "[100,101,102]"s 
			};

			process_json(mock_task1);
			json mock_task2 = task { 
				.name = "child"s, 
				.type = "python"s, 
				.filename = "test.py"s,
				.file_content = "[100,101,102]"s,
				.dependency_names = { { "root"s } }
			};

			process_json(mock_task2);

		}

		auto write_to_file(task const& request) noexcept -> bool {
			return write_to_file(request.filename.value(), request.file_content.value());
		}


		void shy_guy::process_json(json const& json_req) {
			// this is a function		
			auto request = json_req.template get<jx::task>();
			spdlog::info("Name String {} ", request.name);

			// this is a function
			if (not write_to_file(request)) {
				spdlog::warn("failed to write file: {}", request.filename.value());
			}

			// this is a function
			std::string command = "python3 " + request.filename.value();
			auto aaaa = jx::execute(command);
			spdlog::info("executing {} output {}", request.filename.value(), aaaa.output);
		}

		


		auto shy_guy::run() noexcept -> void {

			// read from config file
			enum index { identity, delimiter, data };
			enum errors { no_error, bad_message, try_again };

			zmq::context_t context(1);
			zmq::socket_t responder(context, zmq::socket_type::router);
			zmq::socket_t task_publisher(context, zmq::socket_type::pub);

			responder.bind("tcp://127.0.0.1:" + this->port_);
			task_publisher.bind("inproc://task_handler");

			auto receive_task = [&] (auto& pack) mutable noexcept {
				try {
					return zmq::recv_multipart(responder,  pack.data());
				}
				catch (std::runtime_error const& error) {
					spdlog::error("failed to recv message : {} ", error.what());
					return std::optional<size_t>{};
				}
			};
			auto reply_to_task = [&](auto& pack) mutable noexcept {
				try {
					return zmq::send_multipart(responder, pack);
				}
				catch (std::runtime_error const& error) {
					spdlog::error("failed to recv message : {} ", error.what());
					return std::optional<size_t>{};
				}
			};

			while (true)
			{
				std::array message_pack { zmq::message_t(5), zmq::message_t(), zmq::message_t(100) };
				
				if (auto const size = receive_task(message_pack); size)
				{
					auto const binary_request = message_pack[data].to_string_view();
					auto const unpacked_json  = json::from_bson(binary_request);
					auto const task           = unpacked_json.template get<jx::task>();
					// record json + task					
					
					if (not has_dependencies(task)) {
						//auto const task_graph  = directed_acyclic_graph{ task }; // if it this constructor called means task is root.
						//auto const directory   = make_directory(task_graph);
						// record graph + directory
						// 
						// task_maintainer.emplace_graph(std::move(task_graph));
						
						// start task graph runner 
						// graph runner { .id=(ZMQ_FILTER), .name=(root name?), .dag=(task_graph)  .directory=(directory), .tasks={task} };

						continue;
					}

					
					//	if (const auto root{ find_valid_root_task(task) }; root.has_value()) {
					//		const auto tasks{ root_dependancies.find(root.value()) };
					//  }
					
					 //auto graph_result = check(task)
					//message[data]     = try_task(task);
					//auto 		responder.send();

					
					
					
				}

				// this is a function

				

			
				
				//  Send reply back to client
				reply_to_task(message_pack);
			}
		};

	}
}