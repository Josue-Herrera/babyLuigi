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
#include <cron_expression.hpp>
#include "zmq_consumer.hpp"
#include "process/file_write.hpp"

// Standard Inc
#include <string>
#include <unordered_map>

namespace cosmos {
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
			auto request = json_req.template get<cosmos::task>();
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

		auto shy_guy::run() noexcept -> void {

			// read from config file
			enum index { identity, data };
			enum errors { no_error, bad_message, try_again };

			zmq::context_t context(1);
			zmq::socket_t responder(context, zmq::socket_type::router);

			responder.bind("tcp://127.0.0.1:" + arguments_.port);

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
					auto const task           = unpacked_json.template get<cosmos::task>();
					spdlog::info("request identity {}", request_identity.size(), task.name);
					zmq::send_multipart(responder, message_pack);
				}
			}
		};

		// support API commands
		struct command {
			enum type : uint8_t {
				dag_create,
				dag_delete,
				dag_run,
				dag_snapshot,
				task_create,
				task_delete,
				task_run,
				command_error
			};

			type command_{};
		};

		struct command_result_type {};
		struct notification_type {
		    std::chrono::steady_clock::time_point time;
		    int associated_dag;
		};
		class concurrent_shy_guy {
		public:
		     /**
		      * @brief Create DAG in a concurrent manner
		      * @param input
		      */
		     auto process(command::type input, cosmos::task const& request) noexcept -> command_result_type {
		         std::lock_guard lock (this->mutex_);
		        log->info("dag info {}, {}", std::to_underlying(input), request.name);
		         return {};
		     }
		     /**
		      * @brief run next scheduled dag
		      *
		      * @return time of the next scheduled dag.
		      */
		     auto next_scheduled_dag() const noexcept -> std::optional<notification_type> {
		         std::lock_guard<std::mutex> lock (this->mutex_);
		         return { notification_type {
		             .time = std::chrono::steady_clock::now() +  std::chrono::seconds{5},
		             .associated_dag = 1
		         }};
		     }
		private:
		     mutable std::mutex mutex_{};
		     std::shared_ptr<spdlog::logger> log;
		};
	}
}
