// Project Inc
#include <task.hpp>
#include <shy_guy.hpp>
#include <process/system_execution.hpp>
#include <process/binary_file_conversion.hpp>
#include <process/file_write.hpp>

// 3rdparty Inc
#include<zmq.hpp>
#include <nlohmann/json.hpp>

// Standard Inc
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>

namespace jx {
	inline namespace v1 {
		using namespace std::string_literals;		
		using json = nlohmann::json; 

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