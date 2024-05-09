// Project Inc
#include <task.hpp>
#include <shy_guy.hpp>
#include <process/system_execution.hpp>
#include <process/binary_file_conversion.hpp>
#include <process/file_write.hpp>
#include <task_system/task_queue.hpp>

// 3rdparty Inc
#include <zmq.hpp>
#include <range/v3/all.hpp>
#include <nlohmann/json.hpp>
#include <tl/expected.hpp>

// Standard Inc
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <unordered_map>

namespace jx {
	inline namespace v1 {
		using namespace std::string_literals;		
		using json = nlohmann::json; 

		template<class Request, class Reply> class channel {
		public:
	
			[[nodiscard]] auto wait_for_reply() noexcept -> Reply {
				return reply.dequeue();
			}

			[[nodiscard]] auto wait_for_request() noexcept -> Request {
				return request.dequeue();
			}

			auto send_request(Request const& req) noexcept -> void {
				request.enqueue(req);
			}

	
			auto send_reply(Reply const& rep) noexcept -> void {
				reply.enqueue(rep);
			}

		private:
			task_queue<Request> request{};
			task_queue<Reply>   reply{};
		};



		class task_session {
		public:
			task_session(channel<task, json>& gc, task req) noexcept
				: graph_channel{ gc }, request{ std::move(req) } {}

			task_session(channel<task, json>& gc, json const req) noexcept
				: graph_channel{ gc }, request{ req.template get<jx::task>() } {}

			[[nodiscard]] auto run_session() noexcept -> json {
				this->graph_channel.send_request(request);
				return this->graph_channel.wait_for_reply();
			}
		private:
			channel<task, json>& graph_channel;
			task request{};
		};

		class task_processor : task_system {

			using directed_graphs_map = std::unordered_map<std::string, std::string>;
			using root_graphs_map     = std::unordered_map<std::string, directed_graphs_map>;
			using root_dependency_map = std::unordered_map<std::string, std::vector<std::string>>;

			enum class graph_error { dependencies_not_found, constains_duplicates, task_creates_cycle };
			enum class graph_color { white, gray, black };

		public:
			
			[[nodiscard]] auto channel_reference() noexcept -> channel<task, json>& {
				return this->graph_channel;
			}

			auto run(void) -> void {
				while (true) {
					const auto request = graph_channel.wait_for_request();
	
					if (has_dependencies(request)) {
						const auto root = find_root_task(request);

						
					}
					else {
						root_dependancies.emplace(request.name, std::vector<std::string>{});

						// run root task async()
					}
				}
			}

		private:

			auto find_root_task(jx::task const& request) -> tl::expected<std::string, graph_error> {
				for (auto& [root, tasks] : root_dependancies) {

					auto found = std::find_first_of(tasks.begin(), tasks.end(),
						request.dependency_names.value().begin(),
						request.dependency_names.value().end());

					if (found != tasks.end()) {

						if (has_cycle(root, request.name))
							return tl::unexpected(graph_error::task_creates_cycle);

						if (ranges::contains(tasks, request.name)) // task already exists
							return tl::unexpected(graph_error::constains_duplicates);
						
						return { *found };

						break;
					}
				}
				return tl::unexpected(graph_error::dependencies_not_found);
			}

			auto has_cycle(std::string const& root, std::string const& name) -> bool { 
			
				return false; 
			
			}


			root_graphs_map     graphs{};
			channel<task, json> graph_channel{};
			root_dependency_map root_dependancies{};
		};


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



		void shy_guy::process_json(json const& json_req) {
			// this is a function		
			auto request = json_req.template get<jx::task>();
			spdlog::info("Name String {} ", request.name);

			// this is a function
			if (not write_to_file(request.filename.value(), request.file_content.value())) {
				spdlog::warn("failed to write file: {}", request.filename.value());
			}

			// this is a function
			std::string command = "python3 " + request.filename.value();
			auto aaaa = jx::execute(command);
			spdlog::info("executing {} output {}", request.filename.value(), aaaa.output);
		}

		auto shy_guy::run() noexcept -> void {

	
			zmq::context_t context(1);
			zmq::socket_t responder(context, zmq::socket_type::rep);
			responder.bind("tcp://127.0.0.1:" + this->port_);
			
			while (true)
			{
				// this is a function
				zmq::message_t request{ };
				auto result = responder.recv(request);
				if(!result) continue;
				json json_req = json::from_bson(request.to_string_view());

				process_json(json_req);
				
				//  Send reply back to client
				responder.send(zmq::message_t{ "WORLD"s }, zmq::send_flags::none);
			}
		};

	}
}