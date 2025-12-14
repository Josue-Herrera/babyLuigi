#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "fs_storage.hpp"

namespace fs = std::filesystem;
using namespace cosmos; // uses inline v1

namespace {

// Create a unique temporary directory for each test run under the system temp
struct temp_dir_guard {
  fs::path path;
  explicit temp_dir_guard(std::string prefix)
  {
    auto base = fs::temp_directory_path();
    // Append a pseudo-unique suffix using the address of this
    path = base / (prefix + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
    std::error_code ec;
    fs::create_directories(path, ec);
  }
  ~temp_dir_guard()
  {
    std::error_code ec;
    fs::remove_all(path, ec);
  }
};

inline std::vector<std::byte> to_bytes(std::string const& s)
{
  std::vector<std::byte> out(s.size());
  std::memcpy(out.data(), s.data(), s.size());
  return out;
}

} // namespace

TEST_CASE("fs_blob_store put/get/has works", "[fs_storage][blob]")
{
  temp_dir_guard tmp{"fs_storage_blob_"};
  fs_blob_store blobs{tmp.path};

  const auto hello_bytes = to_bytes("hello world");
  auto id_expected = blobs.put_blob(hello_bytes);
  REQUIRE(id_expected.has_value());
  auto id = *id_expected;

  // FNV-1a 64-bit hex -> 16 hex chars
  REQUIRE(id.size() == 16);

  auto has1 = blobs.has_blob(id);
  REQUIRE(has1.has_value());
  REQUIRE(has1.value());

  auto bytes_expected = blobs.get_blob(id);
  REQUIRE(bytes_expected.has_value());
  auto bytes = *bytes_expected;
  REQUIRE(bytes.size() == hello_bytes.size());
  REQUIRE(std::memcmp(bytes.data(), hello_bytes.data(), bytes.size()) == 0);

  // Idempotent: same content -> same id
  auto id2_expected = blobs.put_blob(hello_bytes);
  REQUIRE(id2_expected.has_value());
  REQUIRE(*id2_expected == id);

  // Different content -> different id
  const auto other_bytes = to_bytes("another content");
  auto id3_expected = blobs.put_blob(other_bytes);
  REQUIRE(id3_expected.has_value());
  REQUIRE(*id3_expected != id);

  // Not found on bad id
  auto bad = blobs.get_blob("ffffffffffffffff");
  REQUIRE_FALSE(bad.has_value());
  REQUIRE(bad.error() == storage_error::not_found);
}

TEST_CASE("fs_dag_store CRUD + versions", "[fs_storage][dag]")
{
  temp_dir_guard tmp{"fs_storage_dag_"};
  fs_dag_store dags{tmp.path};

  shyguy_dag d1{.name = "dag1", .schedule = std::nullopt};

  // Create -> version 1
  auto v1 = dags.upsert_dag(d1);
  REQUIRE(v1.has_value());
  REQUIRE(v1.value() == 1);

  // Read back
  auto got = dags.get_dag("dag1");
  REQUIRE(got.has_value());
  REQUIRE(got->version == 1);
  REQUIRE(got->value.name == "dag1");

  // Update with correct if_version -> version 2
  auto v2 = dags.upsert_dag(d1, 1);
  REQUIRE(v2.has_value());
  REQUIRE(v2.value() == 2);

  // Conflict on wrong if_version
  auto conflict = dags.upsert_dag(d1, 1);
  REQUIRE_FALSE(conflict.has_value());
  REQUIRE(conflict.error() == storage_error::conflict);

  // List contains our dag
  auto list = dags.list_dags();
  REQUIRE(list.has_value());
  REQUIRE_FALSE(list->empty());
  REQUIRE(std::any_of(list->begin(), list->end(), [](dag_entry const& e){ return e.value.name == "dag1"; }));

  // Erase wrong version -> conflict
  auto del_conflict = dags.erase_dag("dag1", 1);
  REQUIRE_FALSE(del_conflict.has_value());
  REQUIRE(del_conflict.error() == storage_error::conflict);

  // Erase with correct version -> ok
  auto del_ok = dags.erase_dag("dag1", 2);
  REQUIRE(del_ok.has_value());

  // Get after delete -> not_found
  auto after = dags.get_dag("dag1");
  REQUIRE_FALSE(after.has_value());
  REQUIRE(after.error() == storage_error::not_found);

  // Delete non-existent -> not_found
  auto del_missing = dags.erase_dag("dag1", std::nullopt);
  REQUIRE_FALSE(del_missing.has_value());
  REQUIRE(del_missing.error() == storage_error::not_found);
}

TEST_CASE("fs_task_store CRUD + versions + blob_id", "[fs_storage][task]")
{
  temp_dir_guard tmp{"fs_storage_task_"};
  fs_task_store tasks{tmp.path};

  shyguy_task t1{};
  t1.name = "task1";
  t1.associated_dag = "dagA";
  t1.type = std::string{"shell"};
  t1.filename = std::string{"script.sh"};
  t1.file_content = std::string{"echo hi"};

  // Create with if_version omitted
  auto v1 = tasks.upsert_task(t1, std::string{"abc123"});
  REQUIRE(v1.has_value());
  REQUIRE(v1.value() == 1);

  // Get back, check blob_id
  auto got = tasks.get_task("dagA", "task1");
  REQUIRE(got.has_value());
  REQUIRE(got->version == 1);
  REQUIRE(got->value.name == "task1");
  REQUIRE(got->value.associated_dag == "dagA");
  REQUIRE(got->blob_id.has_value());
  REQUIRE(got->blob_id.value() == "abc123");

  // List shows our task
  auto list = tasks.list_tasks("dagA");
  REQUIRE(list.has_value());
  REQUIRE(list->size() == 1);
  REQUIRE(list->front().value.name == "task1");

  // Update with correct version -> version 2
  auto v2 = tasks.upsert_task(t1, std::nullopt, 1);
  REQUIRE(v2.has_value());
  REQUIRE(v2.value() == 2);

  // Conflict on wrong version
  auto conflict = tasks.upsert_task(t1, std::nullopt, 1);
  REQUIRE_FALSE(conflict.has_value());
  REQUIRE(conflict.error() == storage_error::conflict);

  // Erase wrong version -> conflict
  auto del_conflict = tasks.erase_task("dagA", "task1", 1);
  REQUIRE_FALSE(del_conflict.has_value());
  REQUIRE(del_conflict.error() == storage_error::conflict);

  // Erase correct version -> ok
  auto del_ok = tasks.erase_task("dagA", "task1", 2);
  REQUIRE(del_ok.has_value());

  // Get after delete -> not_found
  auto after = tasks.get_task("dagA", "task1");
  REQUIRE_FALSE(after.has_value());
  REQUIRE(after.error() == storage_error::not_found);

  // Create with if_version==0 allowed for non-existent
  auto v1_again = tasks.upsert_task(t1, std::nullopt, 0);
  REQUIRE(v1_again.has_value());
  REQUIRE(v1_again.value() == 1);

  // Delete non-existent task
  auto del_missing = tasks.erase_task("dagA", "nope", std::nullopt);
  REQUIRE_FALSE(del_missing.has_value());
  REQUIRE(del_missing.error() == storage_error::not_found);
}

TEST_CASE("fs_task_store stores schedule metadata", "[fs_storage][task][metadata]")
{
  temp_dir_guard tmp{"fs_storage_task_meta_"};
  fs_task_store tasks{tmp.path};

  shyguy_task t1{};
  t1.name = "task_meta";
  t1.associated_dag = "dagMeta";

  task_metadata metadata{};
  metadata.schedule.cron_expression = "0 * * * *";
  metadata.schedule.frequency = schedule_frequency::hourly;
  metadata.statuses.previous = dag_run_status::success;
  metadata.statuses.current  = dag_run_status::running;

  auto created = tasks.upsert_task(t1, std::nullopt, std::nullopt, metadata);
  REQUIRE(created.has_value());

  auto loaded = tasks.get_task("dagMeta", "task_meta");
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->metadata.has_value());
  auto const &meta = loaded->metadata.value();
  REQUIRE(meta.schedule.cron_expression == metadata.schedule.cron_expression);
  REQUIRE(meta.schedule.frequency == metadata.schedule.frequency);
  REQUIRE(meta.statuses.previous == metadata.statuses.previous);
  REQUIRE(meta.statuses.current == metadata.statuses.current);

  auto updated = tasks.upsert_task(t1, std::nullopt, created.value());
  REQUIRE(updated.has_value());
  auto reloaded = tasks.get_task("dagMeta", "task_meta");
  REQUIRE(reloaded.has_value());
  REQUIRE(reloaded->metadata.has_value());
  REQUIRE(reloaded->metadata->schedule.frequency == metadata.schedule.frequency);
}

TEST_CASE("fs_storage aggregates stores with shared root", "[fs_storage][api]")
{
  temp_dir_guard tmp{"fs_storage_root_"};
  fs_storage store{tmp.path};

  // Sanity: use blobs via wrapper
  auto id = store.blobs().put_blob(to_bytes("x"));
  REQUIRE(id.has_value());
  auto has = store.blobs().has_blob(*id);
  REQUIRE(has.has_value());
  REQUIRE(has.value());

  // DAGs and Tasks are reachable
  shyguy_dag d{.name = "D"};
  REQUIRE(store.dags().upsert_dag(d).has_value());
  shyguy_task t{.name = "T", .associated_dag = "D"};
  REQUIRE(store.tasks().upsert_task(t).has_value());
}
