#pragma once

// *** 3rd Party Includes ***
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
#include <task_system/task_system.hpp>
#include <task_system/task_maintainer.hpp>
#include <task_system/task_queue.hpp>
#include <fstream>
#include <process/binary_file_conversion.hpp>
#include <nlohmann/json.hpp>


namespace jx {
inline namespace v1 {
	
class shy_guy {

public:
	auto run() noexcept -> void;
	auto test_run() noexcept -> void;

	explicit shy_guy(std::string port) : port_{port}{};

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
  jx::task_maintainer_type tasks_{};
  std::thread graph_thread_;


  std::vector<std::thread> task_runners{};
  std::atomic_bool terminate_{};
};

} // namespace v1
} // namespace jx
