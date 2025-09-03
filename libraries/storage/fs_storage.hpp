// Filesystem-backed storage (MVP)
#pragma once

#include <filesystem>
#include <mutex>

#include "storage.hpp"

namespace cosmos::inline v1 {

class fs_blob_store : public iblob_store {
public:
  explicit fs_blob_store(std::filesystem::path root);
  std::expected<std::string, storage_error> put_blob(std::span<const std::byte> bytes) override;
  std::expected<std::vector<std::byte>, storage_error> get_blob(std::string_view id) const override;
  std::expected<bool, storage_error> has_blob(std::string_view id) const override;

private:
  std::filesystem::path root_;
  std::filesystem::path blobs_dir_;
};

class fs_dag_store : public idag_store {
public:
  explicit fs_dag_store(std::filesystem::path root);
  std::expected<uint64_t, storage_error> upsert_dag(shyguy_dag const& dag,
                                                    std::optional<uint64_t> if_version) override;
  std::expected<dag_entry, storage_error> get_dag(std::string_view name) const override;
  std::expected<std::vector<dag_entry>, storage_error> list_dags() const override;
  std::expected<void, storage_error> erase_dag(std::string_view name,
                                               std::optional<uint64_t> if_version) override;

private:
  std::filesystem::path root_;
  std::filesystem::path dag_dir_;
  mutable std::mutex mtx_;
};

class fs_task_store : public itask_store {
public:
  explicit fs_task_store(std::filesystem::path root);
  std::expected<uint64_t, storage_error> upsert_task(shyguy_task const& task,
                                                     std::optional<uint64_t> if_version,
                                                     std::optional<std::string> blob_id) override;
  std::expected<task_entry, storage_error> get_task(std::string_view dag, std::string_view name) const override;
  std::expected<std::vector<task_entry>, storage_error> list_tasks(std::string_view dag) const override;
  std::expected<void, storage_error> erase_task(std::string_view dag, std::string_view name,
                                                std::optional<uint64_t> if_version) override;

private:
  std::filesystem::path root_;
  std::filesystem::path task_dir_;
  mutable std::mutex mtx_;
};

class fs_storage : public istorage {
public:
  explicit fs_storage(std::filesystem::path root);
  std::shared_ptr<iblob_store> blobs() override { return blobs_; }
  std::shared_ptr<idag_store> dags() override { return dags_; }
  std::shared_ptr<itask_store> tasks() override { return tasks_; }

private:
  std::filesystem::path root_;
  std::shared_ptr<fs_blob_store> blobs_;
  std::shared_ptr<fs_dag_store> dags_;
  std::shared_ptr<fs_task_store> tasks_;
};

// Helper factory
std::shared_ptr<istorage> make_fs_storage(std::filesystem::path const& root);

} // namespace cosmos::inline v1

