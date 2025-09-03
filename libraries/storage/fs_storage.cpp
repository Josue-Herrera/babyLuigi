#include "fs_storage.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace fs = std::filesystem;

namespace cosmos::inline v1 {

// Very simple non-cryptographic hex hash (FNV-1a 64-bit) for IDs
static std::string fnv1a_hex(std::span<const std::byte> bytes) {
  uint64_t hash = 1469598103934665603ull;
  for (auto b : bytes) {
    hash ^= static_cast<uint8_t>(b);
    hash *= 1099511628211ull;
  }
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

// ---------- fs_blob_store ----------
fs_blob_store::fs_blob_store(fs::path root) : root_{root}, blobs_dir_{root_/"blobs"} {
  std::error_code ec; fs::create_directories(blobs_dir_, ec);
}

std::expected<std::string, storage_error>
fs_blob_store::put_blob(std::span<const std::byte> bytes) {
  auto id = fnv1a_hex(bytes);
  fs::path p = blobs_dir_/ (id + ".bin");
  std::error_code ec;
  if (!fs::exists(p, ec)) {
    std::ofstream os(p, std::ios::binary);
    if (!os) return std::unexpected(storage_error::io);
    os.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!os) return std::unexpected(storage_error::io);
  }
  return id;
}

std::expected<std::vector<std::byte>, storage_error>
fs_blob_store::get_blob(std::string_view id) const {
  fs::path p = blobs_dir_/ (std::string{id} + ".bin");
  std::ifstream is(p, std::ios::binary);
  if (!is) return std::unexpected(storage_error::not_found);
  std::vector<char> buf((std::istreambuf_iterator<char>(is)), {});
  if (!is && !is.eof()) return std::unexpected(storage_error::io);
  std::vector<std::byte> out(buf.size());
  std::memcpy(out.data(), buf.data(), buf.size());
  return out;
}

std::expected<bool, storage_error>
fs_blob_store::has_blob(std::string_view id) const {
  fs::path p = blobs_dir_/ (std::string{id} + ".bin");
  std::error_code ec; return fs::exists(p, ec);
}

// Helpers for metadata IO
static std::expected<nlohmann::json, storage_error> read_json(fs::path const& p) {
  std::ifstream is(p);
  if (!is) return std::unexpected(storage_error::not_found);
  try {
    nlohmann::json j; is >> j; return j;
  } catch (...) { return std::unexpected(storage_error::corrupt); }
}

static std::expected<void, storage_error> write_json_atomic(fs::path const& p, nlohmann::json const& j) {
  std::error_code ec; fs::create_directories(p.parent_path(), ec);
  auto tmp = p; tmp += ".tmp";
  {
    std::ofstream os(tmp);
    if (!os) return std::unexpected(storage_error::io);
    os << j.dump(2);
    if (!os) return std::unexpected(storage_error::io);
  }
  fs::rename(tmp, p, ec);
  if (ec) return std::unexpected(storage_error::io);
  return {};
}

// ---------- fs_dag_store ----------
fs_dag_store::fs_dag_store(fs::path root) : root_{root}, dag_dir_{root_/"metadata"/"dags"} {
  std::error_code ec; fs::create_directories(dag_dir_, ec);
}

std::expected<uint64_t, storage_error>
fs_dag_store::upsert_dag(shyguy_dag const& dag, std::optional<uint64_t> if_version) {
  std::scoped_lock lk(mtx_);
  fs::path p = dag_dir_/dag.name/"dag.json";
  uint64_t new_version = 1;
  if (fs::exists(p)) {
    auto cur = read_json(p);
    if (!cur) return std::unexpected(cur.error());
    uint64_t ver = cur->value("version", 0);
    if (if_version && ver != *if_version) return std::unexpected(storage_error::conflict);
    new_version = ver + 1;
  } else {
    if (if_version && *if_version != 0) return std::unexpected(storage_error::conflict);
  }

  nlohmann::json j;
  j["version"] = new_version;
  j["value"] = dag;
  auto w = write_json_atomic(p, j);
  if (!w) return std::unexpected(w.error());
  return new_version;
}

std::expected<dag_entry, storage_error>
fs_dag_store::get_dag(std::string_view name) const {
  std::scoped_lock lk(mtx_);
  fs::path p = dag_dir_/std::string{name}/"dag.json";
  auto j = read_json(p);
  if (!j) return std::unexpected(j.error());
  dag_entry e{}; e.version = j->value("version", 0);
  e.value = j->at("value").get<shyguy_dag>();
  return e;
}

std::expected<std::vector<dag_entry>, storage_error>
fs_dag_store::list_dags() const {
  std::scoped_lock lk(mtx_);
  std::vector<dag_entry> out;
  std::error_code ec;
  if (!fs::exists(dag_dir_, ec)) return out;
  for (auto const& d : fs::directory_iterator(dag_dir_)) {
    if (!d.is_directory()) continue;
    auto p = d.path()/"dag.json";
    auto j = read_json(p);
    if (!j) continue;
    dag_entry e{}; e.version = j->value("version", 0);
    try { e.value = j->at("value").get<shyguy_dag>(); } catch (...) { continue; }
    out.push_back(std::move(e));
  }
  return out;
}

std::expected<void, storage_error>
fs_dag_store::erase_dag(std::string_view name, std::optional<uint64_t> if_version) {
  std::scoped_lock lk(mtx_);
  fs::path dir = dag_dir_/std::string{name};
  fs::path p = dir/"dag.json";
  if (fs::exists(p)) {
    if (if_version) {
      auto j = read_json(p);
      if (!j) return std::unexpected(j.error());
      uint64_t ver = j->value("version", 0);
      if (ver != *if_version) return std::unexpected(storage_error::conflict);
    }
    std::error_code ec;
    fs::remove(p, ec);
    fs::remove(dir, ec);
    return {};
  }
  return std::unexpected(storage_error::not_found);
}

// ---------- fs_task_store ----------
fs_task_store::fs_task_store(fs::path root) : root_{root}, task_dir_{root_/"metadata"/"tasks"} {
  std::error_code ec; fs::create_directories(task_dir_, ec);
}

std::expected<uint64_t, storage_error>
fs_task_store::upsert_task(shyguy_task const& task, std::optional<uint64_t> if_version, std::optional<std::string> blob_id) {
  std::scoped_lock lk(mtx_);
  fs::path dir = task_dir_/task.associated_dag;
  fs::path p = dir/(task.name + ".json");
  uint64_t new_version = 1;
  if (fs::exists(p)) {
    auto cur = read_json(p);
    if (!cur) return std::unexpected(cur.error());
    uint64_t ver = cur->value("version", 0);
    if (if_version && ver != *if_version) return std::unexpected(storage_error::conflict);
    new_version = ver + 1;
  } else {
    if (if_version && *if_version != 0) return std::unexpected(storage_error::conflict);
  }
  nlohmann::json j;
  j["version"] = new_version;
  j["value"] = task;
  if (blob_id) j["blob_id"] = *blob_id;
  auto w = write_json_atomic(p, j);
  if (!w) return std::unexpected(w.error());
  return new_version;
}

std::expected<task_entry, storage_error>
fs_task_store::get_task(std::string_view dag, std::string_view name) const {
  std::scoped_lock lk(mtx_);
  fs::path p = task_dir_/std::string{dag}/(std::string{name} + ".json");
  auto j = read_json(p);
  if (!j) return std::unexpected(j.error());
  task_entry e{}; e.version = j->value("version", 0);
  e.value = j->at("value").get<shyguy_task>();
  if (j->contains("blob_id")) e.blob_id = j->value("blob_id", "");
  return e;
}

std::expected<std::vector<task_entry>, storage_error>
fs_task_store::list_tasks(std::string_view dag) const {
  std::scoped_lock lk(mtx_);
  fs::path dir = task_dir_/std::string{dag};
  std::vector<task_entry> out;
  std::error_code ec;
  if (!fs::exists(dir, ec)) return out;
  for (auto const& f : fs::directory_iterator(dir)) {
    if (!f.is_regular_file()) continue;
    auto j = read_json(f.path());
    if (!j) continue;
    task_entry e{}; e.version = j->value("version", 0);
    try { e.value = j->at("value").get<shyguy_task>(); } catch (...) { continue; }
    if (j->contains("blob_id")) e.blob_id = j->value("blob_id", "");
    out.push_back(std::move(e));
  }
  return out;
}

std::expected<void, storage_error>
fs_task_store::erase_task(std::string_view dag, std::string_view name, std::optional<uint64_t> if_version) {
  std::scoped_lock lk(mtx_);
  fs::path p = task_dir_/std::string{dag}/(std::string{name} + ".json");
  if (!fs::exists(p)) return std::unexpected(storage_error::not_found);
  if (if_version) {
    auto j = read_json(p);
    if (!j) return std::unexpected(j.error());
    uint64_t ver = j->value("version", 0);
    if (ver != *if_version) return std::unexpected(storage_error::conflict);
  }
  std::error_code ec; fs::remove(p, ec); if (ec) return std::unexpected(storage_error::io);
  return {};
}

// ---------- fs_storage ----------
fs_storage::fs_storage(fs::path root) : root_{root} {
  blobs_ = std::make_shared<fs_blob_store>(root_);
  dags_  = std::make_shared<fs_dag_store>(root_);
  tasks_ = std::make_shared<fs_task_store>(root_);
}

std::shared_ptr<istorage> make_fs_storage(std::filesystem::path const& root) {
  return std::make_shared<fs_storage>(root);
}

} // namespace cosmos::inline v1
