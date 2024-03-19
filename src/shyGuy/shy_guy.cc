#include <shy_guy.hpp>

#include<zmq.hpp>
#include <nlohmann/json.hpp>

#include <chrono>


zmq::message_t sad{};


namespace jx {
	inline namespace v1 {

		using json = nlohmann::json;


		auto shy_guy::run() noexcept -> void {

			zmq::context_t context(1);

			zmq::socket_t responder(context, zmq::socket_type::rep);
			responder.bind("tcp://localhost:5559");
			
			while (true)
			{
				//  Wait for next request from client
				zmq::message_t request{ };

				auto result = responder.recv(request);
				
				json json_req = json::from_bson(request.to_string_view());
				
				std::string s(json_req["payload"].dump());

				spdlog::info("Received request [ {} ]", json_req.dump());

				spdlog::info("Payload String {} ", s);

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