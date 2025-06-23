#pragma once

// *** Project Includes ***
#include "concurrent_shyguy.hpp"
#include "define_arguements.hpp"

// *** 3rd Party Includes ***
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// *** Standard Includes ***
#include <string>
#include <thread>
#include <vector>

namespace cosmos {
inline namespace v1 {
	
class shyguy {

public:
	auto run() noexcept -> void;
	auto test_run() noexcept -> void;

	explicit shyguy(shy_arguments arguments) :
		arguments{std::move(arguments)}{}

    ~shyguy() = default;
	shyguy(shyguy const &) = delete;
	shyguy(shyguy &&) = delete;
	shyguy &operator=(shyguy const &) = delete;
	shyguy &operator=(shyguy &&) = delete;
private:
  void process_json(nlohmann::json const& json_req);

  // context must come firsts
  shy_arguments arguments{};
  std::thread ctx_thread_;

  // handles new connections

  // task maintainer
  std::thread graph_thread_;


  std::vector<std::thread> task_runners{};
  std::atomic_bool terminate_{};
};

} // namespace v1
} // namespace cosmos
