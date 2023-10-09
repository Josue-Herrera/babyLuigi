
#include "task_system/task_queue.hpp"
#include <asio/buffer.hpp>
#include <asio/registered_buffer.hpp>
#include <asio/write.hpp>
#include <iterator>
#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>
#include <string>

#include <shy_guy.hpp>
#include <system_error>

namespace jx {
inline namespace v1 {

using json = nlohmann::json;

auto shy_guy::run() noexcept -> void {
  spdlog::info("shy guy running.");

  try {
    wait_for_connection();

    ctx_thread_ = std::thread{[this] { ctx_.run(); }};

    graph_thread_ = std::thread{[this] {
      while (not terminate_.load()) {
        task_view view = task_queue_.dequeue();

        view.response_queue->enqueue(tasks_.service(view)
                                         ? respose_reason::completing
                                         : respose_reason::revoking);
      }
    }};
  } catch (std::exception const &ex) {
    spdlog::error("Unhandled exception in s: {}", ex.what());
  }
}

auto shy_guy::stop() noexcept -> void {
  ctx_.stop();

  if (ctx_thread_.joinable())
    ctx_thread_.join();

  spdlog::info("shy guy stopping.");
}

auto shy_guy::read_header(asio::ip::tcp::socket &socket) noexcept -> void {
  std::uint64_t message_size{};
  asio::async_read(
      socket, asio::buffer(&message_size, sizeof(message_size)),
      [this, &socket](std::error_code error, std::size_t length) mutable {
        if (error)
          spdlog::warn("reading header error: {}", error.message());
        else {
          read_body(socket, length);
        }
      });
}

auto shy_guy::read_body(asio::ip::tcp::socket &socket,
                        std::size_t body_length) noexcept -> void {
  std::string message{};
  message.resize(body_length);
  asio::async_read(
      socket, asio::buffer(message.data(), message.size()),
      [this, &socket, &message](std::error_code error,
                                [[maybe_unused]] std::size_t) mutable {
        if (error)
          spdlog::warn("reading header error: {}", error.message());
        else {
          auto task_response =
              std::make_shared<jx::task_queue<respose_reason>>();

          task_queue_.enqueue(
              jx::task_view{.request = std::string_view(message),
                            .response = respose_reason::not_set,
                            .response_queue = task_response});

          // data race here because the dequeue can b
          auto response = task_response->dequeue();
          spdlog::info("task respone {}", static_cast<int>(response));

          json j_response{{"response", response}};

          write_header(socket, j_response.dump());
        }
      });
}

auto shy_guy::write_header(asio::ip::tcp::socket &socket,
                           std::string const &message) noexcept -> void {

  std::uint64_t message_size{std::size(message)};

  asio::async_write(
      socket, asio::buffer(&message_size, sizeof(message_size)),
      [this, &socket, &message](std::error_code error, std::size_t size) {
        if (error or size == 0) {
          spdlog::warn("writting body error: {}", error.message());
          socket.close();
        } else {
          write_body(socket, message);
        }
      });
}

auto shy_guy::write_body(asio::ip::tcp::socket &socket,
                         std::string const &message) noexcept -> void {
  asio::async_write(
      socket, asio::buffer(std::data(message), std::size(message)),
      [&socket](std::error_code error, std::size_t size) {
        if (error or size == 0) {
          spdlog::warn("writting body error: {}", error.message());
          socket.close();
        }
      });
}

auto shy_guy::wait_for_connection() noexcept -> void {
  acceptor_.async_accept(
      [this](std::error_code error, asio::ip::tcp::socket socket) mutable {
        if (error)
          spdlog::warn("connection error: {}", error.message());
        else {
          spdlog::info("new connection established {}:{}",
                       socket.remote_endpoint().address().to_string(),
                       socket.remote_endpoint().port());

          // reading header -> then body
          // then wait for response -> send response
          read_header(socket);

          socket.close();
        }

        wait_for_connection();
      });
}

shy_guy::shy_guy(unsigned port)
    : acceptor_(ctx_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(),
                                              static_cast<uint16_t>(port))) {}

shy_guy::~shy_guy() { stop(); }

} // namespace v1
} // namespace jx
