#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <atomic>
#include <optional>

// This test validates end-to-end with the zmq_router, but uses
// a different port to avoid conflicts if tests run in parallel.

#include <babyLuigi/baby_luigi.hpp>
#include <shyGuy/zmq_router.hpp>
#include <shyguy_request.hpp>

using cosmos::shyguy_request;
using cosmos::shyguy_dag;
using cosmos::shy_guy_info;

TEST_CASE("babyLuigi gets OK ACK from router", "[zmq][integration]") {
    constexpr int test_port = 39231;

    auto term = std::make_shared<std::atomic_bool>(false);
    cosmos::v1::request_queue_t rq; // unused for this test
    cosmos::v1::zmq_router router{rq, term, test_port};

    // Prepare a DAG create request
    shyguy_request req{};
    shyguy_dag dag{}; dag.name = "demo";
    req.data = dag;
    req.command = cosmos::v1::command_enum::create;

    // Router waits for the request in background
    std::optional<shyguy_request> received;
    std::thread server([&]{ received = router.route_data(); });

    shy_guy_info info{};
    info.server_address = "127.0.0.1";
    info.port = std::to_string(test_port);

    // Send using babyLuigi API (expects OK ack)
    REQUIRE(cosmos::submit_task_request(req, info));

    server.join();

    REQUIRE(received.has_value());
    REQUIRE(cosmos::v1::is_dag(received.value()));
    REQUIRE(received->name() == std::string{"demo"});
}
