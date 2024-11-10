#pragma once

// *** Project Includes ***
#include "task_maintainer.hpp"

// *** 3rd Party Includes ***
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// *** Standard Includes ***
#include <string>
#include <thread>
#include <vector>

namespace cosmos {
inline namespace v1 {
	
class shy_guy {

public:
	auto run() noexcept -> void;
	auto test_run() noexcept -> void;

	explicit shy_guy(std::string port) : port_{std::move(port)}{};

	~shy_guy() = default;
	shy_guy(shy_guy const &) = delete;
	shy_guy(shy_guy &&) = delete;
	shy_guy &operator=(shy_guy const &) = delete;
	shy_guy &operator=(shy_guy &&) = delete;
private:
  void process_json(nlohmann::json const& json_req);

  // context must come first
  std::string port_{};
  std::thread ctx_thread_;

  // handles new connections

  // task maintainer
  cosmos::task_maintainer_type tasks_{};
  std::thread graph_thread_;


  std::vector<std::thread> task_runners{};
  std::atomic_bool terminate_{};
};

} // namespace v1
} // namespace cosmos
