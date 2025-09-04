// Filesystem-backed storage (MVP)
#pragma once

#include <filesystem>
#include <mutex>

#include "storage.hpp"

namespace cosmos::inline v1 {

class fs_blob_store
{
public:
  explicit fs_blob_store(std::filesystem::path root);
  [[nodiscard]] auto put_blob(std::span<const std::byte> bytes) const -> std::expected<std::string, storage_error>;
  [[nodiscard]] auto get_blob(std::string_view id) const -> std::expected<std::vector<std::byte>, storage_error>;
  [[nodiscard]] auto has_blob(std::string_view id) const -> std::expected<bool, storage_error>;

private:
  std::filesystem::path root_;
  std::filesystem::path blobs_dir_;
};

class fs_dag_store
{
public:
  explicit fs_dag_store(std::filesystem::path root);

  [[nodiscard]] auto upsert_dag(
    shyguy_dag const& dag,
    std::optional<uint64_t> if_version = std::nullopt
  ) const -> std::expected<uint64_t, storage_error>;

  [[nodiscard]] auto get_dag(std::string_view name) const -> std::expected<dag_entry, storage_error>;

  [[nodiscard]] auto list_dags() const -> std::expected<std::vector<dag_entry>, storage_error>;

  [[nodiscard]] auto erase_dag(
    std::string_view name,
    std::optional<uint64_t> if_version = std::nullopt
  ) const -> std::expected<void, storage_error>;

private:
  std::filesystem::path root_;
  std::filesystem::path dag_dir_;
  mutable std::mutex mtx_;
};

class fs_task_store  {
public:
  explicit fs_task_store(std::filesystem::path root);

  [[nodiscard]] auto upsert_task(
    shyguy_task const& task,
    const std::optional<std::string>& blob_id = std::nullopt,
    std::optional<uint64_t> if_version = std::nullopt
    ) const -> std::expected<uint64_t, storage_error>;

  [[nodiscard]] auto get_task(
    std::string_view dag,
    std::string_view name
    ) const -> std::expected<task_entry, storage_error>;

  [[nodiscard]] auto list_tasks(
    std::string_view dag) const
  -> std::expected<std::vector<task_entry>, storage_error>;

  [[nodiscard]] auto erase_task(
    std::string_view dag,
    std::string_view name,
    std::optional<uint64_t> if_version = std::nullopt
  ) const -> std::expected<void, storage_error>;

private:
  std::filesystem::path root_;
  std::filesystem::path task_dir_;
  mutable std::mutex mtx_;
};

class fs_storage {



public:
  explicit fs_storage(const std::filesystem::path& root);
  [[nodiscard]] fs_blob_store const& blobs() const { return blobs_; }
  [[nodiscard]] fs_dag_store const& dags() const { return dags_; }
  [[nodiscard]] fs_task_store const& tasks() const { return tasks_; }

private:
  std::filesystem::path root_;
  fs_blob_store blobs_;
  fs_dag_store dags_;
  fs_task_store tasks_;
};

static_assert(is_storageable_v<fs_storage>, "Must implement storage interface");

} // namespace cosmos::inline v1

