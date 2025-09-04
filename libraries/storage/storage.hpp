// Storage interfaces for DAGs and Tasks (snake_case classes)
#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <span>

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

template<class blob_store>
concept blob_storeable = requires(blob_store b, std::span<const std::byte> bytes, std::string_view id) {
  { b.put_blob(bytes) } -> std::same_as<std::expected<std::string, storage_error>>;
  { b.get_blob(id) } -> std::same_as<std::expected<std::vector<std::byte>, storage_error>>;
  { b.has_blob(id) } -> std::same_as<std::expected<bool, storage_error>>;
};

template<class dag_store>
concept dag_storeable = requires(dag_store d, shyguy_dag const& dag, std::string_view name) {
  { d.upsert_dag(dag, std::optional<uint64_t>{}) }  -> std::same_as<std::expected<uint64_t, storage_error>>;
  { d.get_dag(name) } -> std::same_as<std::expected<dag_entry, storage_error>>;
  { d.list_dags() } -> std::same_as<std::expected<std::vector<dag_entry>, storage_error>>;
  { d.erase_dag(name, std::optional<uint64_t>{}) } -> std::same_as<std::expected<void, storage_error>>;
};

template<class task_store>
concept task_storeable = requires(task_store t, shyguy_task const& task, std::string_view dag, std::string_view name)
{
  { t.upsert_task(task, std::optional<std::string>{}, std::optional<uint64_t>{}) } -> std::same_as<std::expected<uint64_t, storage_error>>;
  { t.get_task(dag, name) } -> std::same_as<std::expected<task_entry, storage_error>>;
  { t.list_tasks(dag) } -> std::same_as<std::expected<std::vector<task_entry>, storage_error>>;
  { t.erase_task(dag, name, std::optional<uint64_t>{}) } -> std::same_as<std::expected<void, storage_error>>;
};

template<class storage>
concept storageable = requires(storage s)
{
  { s.blobs() } -> blob_storeable;
  { s.dags() }  -> dag_storeable;
  { s.tasks() } -> task_storeable;
};

template<class storage>
constexpr bool is_storageable_v = storageable<storage>;

} // namespace cosmos::inline v1