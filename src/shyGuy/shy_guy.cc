#include <shy_guy.hpp>
#include <process/system_execution.hpp>

#include<zmq.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <ranges>



namespace jx {
	inline namespace v1 {

		using json = nlohmann::json;

		namespace ranges = std::ranges;

		namespace views = std::views;

		std::string binary_to_file(const std::string& input) {
			std::string result;

			// Remove '[' and ']'
			std::string cleanInput = input.substr(1, input.size() - 2);

			// Split the string by ','
			std::vector<std::string> tokens;
			size_t pos = 0;
			while ((pos = cleanInput.find(',')) != std::string::npos) {
				tokens.push_back(cleanInput.substr(0, pos));
				cleanInput.erase(0, pos + 1);
			}
			tokens.push_back(cleanInput); // Last token

			// Convert tokens to char values
			for (const auto& token : tokens) {
				int value = std::stoi(token);
				char ch = static_cast<char>(value);
				result += ch;
			}

			return result;
		}

		auto shy_guy::run() noexcept -> void {

			using namespace std::string_literals;

			zmq::context_t context(1);

			zmq::socket_t responder(context, zmq::socket_type::rep);
			responder.bind("tcp://127.0.0.1:5559");
			
			while (true)
			{
				//  Wait for next request from client
				zmq::message_t request{ };

				auto result = responder.recv(request);
				
				json json_req = json::from_bson(request.to_string_view());
				
				auto payload = json_req["payload"].dump();

				auto clean = [](auto load) {
					std::erase_if(load, [](auto i){ return (i == '[') || (i == ']');});
					return load;
				};

				payload = binary_to_file(payload);
				
				auto type = json_req["type"].dump();
				
				auto extension = (type == "\"python\"") ? ".py"s : ".sh"s;

				auto name = json_req["name"].dump();

				std::erase(name, '\"');

				auto filename = name + extension;
				
				spdlog::info("Name String {} ", filename);

				std::ofstream ofs(filename, std::ios::out | std::ios::binary);
				
				if (ofs.is_open()) {
					ofs.write((char*)payload.data(), payload.size());
					ofs.close();
				}

				std::string command = "python3 " + filename;
				auto aaaa = jx::execute(command);

				spdlog::info("executing {}", filename);

				spdlog::info("result \n {}", aaaa.output);
				
				spdlog::info("Received request [ {} ]", json_req.dump());

				// Do some 'work'
				std::this_thread::sleep_for(std::chrono::seconds{1});

				//  Send reply back to client
				responder.send(zmq::message_t{ "WORLD" });
			}

			/*graph_thread_ = std::thread{ [this] {

				while (not terminate_.load()) {

					task_view view = task_queue_.dequeue();

					view.response_queue->enqueue(tasks_.service(view)
												? respose_reason::completing
												: respose_reason::revoking);
				}
			}};*/



		};

	}
}