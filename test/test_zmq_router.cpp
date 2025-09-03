#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <atomic>
#include <optional>

#include <nlohmann/json.hpp>

#include <shyGuy/zmq_router.hpp>
#include <babyLuigi/baby_luigi.hpp>
#include <shyguy_request.hpp>

using cosmos::v1::shyguy_request;
using cosmos::v1::shyguy_dag;
using cosmos::v1::shy_guy_info;

TEST_CASE("zmq_router replies with ACK and returns request", "[zmq][router]") {
    // Choose a test port unlikely to collide
    constexpr int test_port = 39221;

    auto terminator = std::make_shared<std::atomic_bool>(false);
    cosmos::v1::request_queue_t rq; // not used by route_data in current test
    cosmos::v1::zmq_router router{rq, terminator, test_port};

    // Prepare request
    shyguy_request req{};
    shyguy_dag dag{}; dag.name = "ack_demo";
    req.data = dag;
    req.command = cosmos::v1::command_enum::create;

    // Launch receiver in a thread to block waiting for message
    std::optional<shyguy_request> received;
    std::thread t([&]{
        received = router.route_data();
    });

    // Send using baby Luigi API to the router
    shy_guy_info info{};
    info.server_address = "127.0.0.1";
    info.port = std::to_string(test_port);

    REQUIRE(cosmos::v1::submit_task_request(req, info));

    t.join();

    REQUIRE(received.has_value());
    REQUIRE(cosmos::v1::is_dag(received.value()));
    REQUIRE(received->name() == std::string{"ack_demo"});
}

