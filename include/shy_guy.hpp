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
#include <task.hpp>
#include <task_system/task_system.hpp>
#include <task_system/task_maintainer.hpp>
#include <task_system/task_queue.hpp>
#include <fstream>
#include <process/binary_file_conversion.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
inline void test_json(std::string const& a) {
	json s = json::parse(a);

	auto ab = s["payload"]
		| ranges::views::transform([](auto const& i) { return  i.template get<char>(); })
		| ranges::to<std::string>();

	spdlog::info("{}", ab);

	for (auto const& i : s["payload"])
	{
		spdlog::info("{}", i.template get<char>());
	}
}



namespace jx {
inline namespace v1 {
	
class shy_guy {

public:
	auto run() noexcept -> void;

	shy_guy(unsigned port) {};

	~shy_guy() = default;
	shy_guy(shy_guy const &) = delete;
	shy_guy(shy_guy &&) = delete;
	shy_guy &operator=(shy_guy const &) = delete;
	shy_guy &operator=(shy_guy &&) = delete;
private:
  // context must come first

  std::thread ctx_thread_;

  // handles new connections

  // task maintainer
  jx::task_maintainer tasks_{};
  jx::task_queue<task_view> task_queue_{};
  std::thread graph_thread_;

  std::atomic_bool terminate_{};
};

} // namespace v1
} // namespace jx
