// Storage interfaces for DAGs and Tasks (snake_case classes)
#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <span>

#include <nlohmann/json.hpp>

#include "shyguy_request.hpp" // for shyguy_dag, shyguy_task

namespace cosmos::inline v1 {

// Basic storage error taxonomy
enum class storage_error : uint8_t {
  not_found,
  already_exists,
  conflict,
  io,
  corrupt,
  transient,
  permission,
  unsupported
};

struct dag_entry {
  shyguy_dag value{};
  uint64_t version{0};
};

struct task_entry {
  shyguy_task value{};
  uint64_t version{0};
  // optional blob id that points to task content
  std::optional<std::string> blob_id{};
};

class iblob_store {
public:
  virtual ~iblob_store() = default;
  virtual std::expected<std::string, storage_error> put_blob(std::span<const std::byte> bytes) = 0;
  virtual std::expected<std::vector<std::byte>, storage_error> get_blob(std::string_view id) const = 0;
  virtual std::expected<bool, storage_error> has_blob(std::string_view id) const = 0;
};

class idag_store {
public:
  virtual ~idag_store() = default;
  virtual std::expected<uint64_t, storage_error> upsert_dag(shyguy_dag const& dag,
                                                            std::optional<uint64_t> if_version = std::nullopt) = 0;
  virtual std::expected<dag_entry, storage_error> get_dag(std::string_view name) const = 0;
  virtual std::expected<std::vector<dag_entry>, storage_error> list_dags() const = 0;
  virtual std::expected<void, storage_error> erase_dag(std::string_view name,
                                                       std::optional<uint64_t> if_version = std::nullopt) = 0;
};

class itask_store {
public:
  virtual ~itask_store() = default;
  virtual std::expected<uint64_t, storage_error> upsert_task(shyguy_task const& task,
                                                             std::optional<uint64_t> if_version = std::nullopt,
                                                             std::optional<std::string> blob_id = std::nullopt) = 0;
  virtual std::expected<task_entry, storage_error> get_task(std::string_view dag,
                                                            std::string_view name) const = 0;
  virtual std::expected<std::vector<task_entry>, storage_error> list_tasks(std::string_view dag) const = 0;
  virtual std::expected<void, storage_error> erase_task(std::string_view dag, std::string_view name,
                                                        std::optional<uint64_t> if_version = std::nullopt) = 0;
};

class istorage {
public:
  virtual ~istorage() = default;
  virtual std::shared_ptr<iblob_store> blobs() = 0;
  virtual std::shared_ptr<idag_store> dags() = 0;
  virtual std::shared_ptr<itask_store> tasks() = 0;
};

} // namespace cosmos::inline v1
