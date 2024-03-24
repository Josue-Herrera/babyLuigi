// Project Inc
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

		using json = nlohmann::json; 

		auto shy_guy::run() noexcept -> void {

			using namespace std::string_literals;

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

				// this is a function		
				auto lol = json_req["payload"].array();
				auto payload = jx::binary_file_conversion(json_req["payload"].dump());
				
				
				// this is a function
				auto name = json_req["name"].dump();
				name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
				auto type = json_req["type"].dump();
				auto extension = (type == "\"python\"") ? ".py"s : ".sh"s;
				auto filename = name + extension;
				spdlog::info("Name String {} ", filename);

				// this is a function
				if (not write_to_file(filename, payload)) {
					spdlog::warn("failed to write file: {}", filename);
				}

				// this is a function
				std::string command = "python3 " + filename;
				auto aaaa = jx::execute(command);
				spdlog::info("executing {} output {}", filename, aaaa.output);
			
				//  Send reply back to client
				responder.send(zmq::message_t{ "WORLD"s }, zmq::send_flags::none);
			}
		};

	}
}