#pragma once

// *** 3rd Party Includes ***
#include <asio.hpp>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// *** Project Includes ***
#include <task.hpp>
#include <task_system/task_system.hpp>
#include <task_system/task_maintainer.hpp>
#include <task_system/task_queue.hpp>

namespace jx {
inline namespace v1 {

class shy_guy {

public:
  auto run() noexcept -> void;

  auto stop() noexcept -> void;

  auto wait_for_connection() noexcept -> void;

  auto read_header(asio::ip::tcp::socket &) noexcept -> void;
  auto read_body(asio::ip::tcp::socket &, std::size_t) noexcept -> void;
  auto write_header(asio::ip::tcp::socket&, std::string const&) noexcept -> void;
  auto write_body(asio::ip::tcp::socket&, std::string const&) noexcept -> void;

  shy_guy(unsigned port);

  ~shy_guy();

  shy_guy(shy_guy const &) = delete;

  shy_guy(shy_guy &&) = delete;

  shy_guy &operator=(shy_guy const &) = delete;

  shy_guy &operator=(shy_guy &&) = delete;

private:
  // context must come first
  asio::io_context ctx_;
  std::thread ctx_thread_;

  // handles new connections
  asio::ip::tcp::acceptor acceptor_;

  // task maintainer
  jx::task_maintainer tasks_{};
  jx::task_queue<task_view> task_queue_{};
  std::thread graph_thread_;

  std::atomic_bool terminate_{};
};

} // namespace v1
} // namespace jx
