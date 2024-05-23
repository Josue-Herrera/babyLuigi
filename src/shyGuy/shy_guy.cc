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
#include <unordered_map>

namespace jx {
	inline namespace v1 {
		using namespace std::string_literals;		
		using json = nlohmann::json; 

		template<class Request, class Reply> class bchannel {
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
			blocking_queue<Request> request{};
			blocking_queue<Reply>   reply{};
		};


		template<class Request, class Reply, template<class> class Queue = blocking_queue>
		class channel {
		public:

			[[nodiscard]] auto recv_reply() noexcept {
				return reply.dequeue();
			}

			[[nodiscard]] auto recv_request() noexcept  {
				return request.dequeue();
			}

			auto send_request(Request const& req) noexcept {
				request.enqueue(req);
			}


			auto send_reply(Reply const& rep) noexcept {
				reply.enqueue(rep);
			}

		private:
			Queue<Request> request{};
			Queue<Reply>   reply{};
		};



		class task_session {
		public:
			task_session(channel<task, json>& gc, task req) noexcept
				: graph_channel{ gc }, request{ std::move(req) } {}

			task_session(channel<task, json>& gc, json const req) noexcept
				: graph_channel{ gc }, request{ req.template get<jx::task>() } {}

			[[nodiscard]] auto run_session() noexcept -> json {
				this->graph_channel.send_request(request);
				return this->graph_channel.recv_reply();
			}
		private:
			channel<task, json>& graph_channel;
			task request{};
		};

		class task_processor : task_system {

			using directed_graphs_map = std::unordered_map<std::string, std::string>;
			using root_graphs_map = std::unordered_map<std::string, directed_graphs_map>;
			using root_dependency_map = std::unordered_map<std::string, std::vector<std::string>>;
			enum class graph_error { not_set, dependencies_not_found, constains_duplicates, task_creates_cycle };
			[[nodiscard]] auto to_cstring(graph_error const error) noexcept {
				switch (error) {
				case graph_error::not_set: return "not_set";
				case graph_error::dependencies_not_found: return "dependencies_not_found";
				case graph_error::constains_duplicates: return "constains_duplicates";
				case graph_error::task_creates_cycle: return "task_creates_cycle";
				default: return "unknown";
				}
			}

		public:
			
			[[nodiscard]] auto channel_reference() noexcept -> channel<task, json>& {
				return this->graph_channel;
			}

			
			auto run(void) -> void {
				async([this] { handle_sessions(); });
				async([this] { handle_tasks(); });
			}

		private:


			auto handle_tasks() -> void {
				while (true) {
					if (const auto request{ task_channel.recv_request() }; request) {

					}

				}
				
			}

			auto handle_sessions() -> void {

				while (true) {
					const auto request{ graph_channel.recv_request() };

					if (not has_dependencies(request)) {
						root_dependancies.emplace(request.name, std::vector<std::string>{});
						task_channel.send_request(request);

						continue;
					}


					if (const auto root { find_valid_root_task(request) }; root.has_value()) {
						const auto tasks { root_dependancies.find(root.value())};

					}
					else spdlog::error("finding root task error: {}", to_cstring(root.error()));
				}
			}


			enum class graph_color { white, gray, black };

			auto has_cycle(std::string const& root, std::string const& name) -> bool {
				return false;
			}


			auto find_valid_root_task(jx::task const& request) -> tl::expected<std::string, graph_error> {

				for (auto& [root, tasks] : root_dependancies) {

					auto found { ranges::find_first_of(tasks, request.dependency_names.value()) };

					if (found != tasks.end()) {

						if (has_cycle(root, request.name))
							return tl::unexpected(graph_error::task_creates_cycle);

						if (ranges::contains(tasks, request.name)) // task already exists
							return tl::unexpected(graph_error::constains_duplicates);

						return { root };
					}
				}

				return tl::unexpected(graph_error::dependencies_not_found);
			}

			root_graphs_map     graphs{};
			channel<task, json> graph_channel{};
			channel<task, json, nonblocking_queue> task_channel{};
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