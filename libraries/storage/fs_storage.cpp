#include "fs_storage.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace cosmos::inline v1
{

    namespace
    {
        using namespace std::string_view_literals;

        constexpr std::array schedule_frequency_names{
            "hourly"sv, "daily"sv, "weekly"sv, "monthly"sv, "yearly"sv, "custom"sv
        };

        constexpr std::array dag_run_status_names{
            "none"sv, "success"sv, "running"sv, "failed"sv, "skipped"sv, "queued"sv
        };

        [[nodiscard]] auto normalize_token(std::string_view value) noexcept -> std::string
        {
            std::string normalized;
            normalized.reserve(value.size());
            for (char ch: value)
            {
                normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            return normalized;
        }

        [[nodiscard]] auto is_star_like(std::string_view token) noexcept -> bool
        {
            return token == "*" || token == "?";
        }

        [[nodiscard]] auto is_simple_value(std::string_view token) noexcept -> bool
        {
            if (token.empty())
                return false;
            return token.find_first_of("*/-,") == std::string_view::npos;
        }

        [[nodiscard]] auto tokenize_cron(std::string_view cron) noexcept -> std::optional<std::array<std::string_view, 5>>
        {
            std::array<std::string_view, 5> tokens{};
            std::size_t index = 0;
            std::size_t pos = 0;
            while (index < tokens.size() and pos < cron.size())
            {
                while (pos < cron.size() and std::isspace(static_cast<unsigned char>(cron[pos])))
                    ++pos;
                if (pos >= cron.size())
                    break;
                std::size_t end = pos;
                while (end < cron.size() and not std::isspace(static_cast<unsigned char>(cron[end])))
                    ++end;
                tokens[index++] = std::string_view{cron.data() + pos, end - pos};
                pos = end;
            }
            if (index != tokens.size())
                return std::nullopt;
            return tokens;
        }
    } // namespace

    auto to_string(schedule_frequency frequency) noexcept -> std::string_view
    {
        using enum schedule_frequency;
        switch (frequency)
        {
            case hourly:  return schedule_frequency_names[0];
            case daily:   return schedule_frequency_names[1];
            case weekly:  return schedule_frequency_names[2];
            case monthly: return schedule_frequency_names[3];
            case yearly:  return schedule_frequency_names[4];
            case custom:
            default:
                return schedule_frequency_names[5];
        }
    }

    auto schedule_frequency_from_string(std::string_view value) noexcept -> schedule_frequency
    {
        auto normalized = normalize_token(value);
        if (normalized == schedule_frequency_names[0]) return schedule_frequency::hourly;
        if (normalized == schedule_frequency_names[1]) return schedule_frequency::daily;
        if (normalized == schedule_frequency_names[2]) return schedule_frequency::weekly;
        if (normalized == schedule_frequency_names[3]) return schedule_frequency::monthly;
        if (normalized == schedule_frequency_names[4]) return schedule_frequency::yearly;
        return schedule_frequency::custom;
    }

    auto classify_schedule(std::string_view cron_expression) noexcept -> schedule_frequency
    {
        if (cron_expression.empty())
            return schedule_frequency::custom;

        auto tokens = tokenize_cron(cron_expression);
        if (not tokens)
            return schedule_frequency::custom;

        auto const &minute = (*tokens)[0];
        auto const &hour   = (*tokens)[1];
        auto const &dom    = (*tokens)[2];
        auto const &month  = (*tokens)[3];
        auto const &dow    = (*tokens)[4];

        if (not is_simple_value(minute))
            return schedule_frequency::custom;

        if (is_star_like(hour) and is_star_like(dom) and is_star_like(month) and is_star_like(dow))
            return schedule_frequency::hourly;

        if (is_simple_value(hour) and is_star_like(dom) and is_star_like(month) and is_star_like(dow))
            return schedule_frequency::daily;

        if (is_simple_value(hour) and is_star_like(dom) and is_star_like(month) and not is_star_like(dow))
            return schedule_frequency::weekly;

        if (is_simple_value(hour) and not is_star_like(dom) and is_star_like(month) and is_star_like(dow))
            return schedule_frequency::monthly;

        if (is_simple_value(hour) and not is_star_like(dom) and not is_star_like(month))
            return schedule_frequency::yearly;

        return schedule_frequency::custom;
    }

    auto to_string(dag_run_status status) noexcept -> std::string_view
    {
        using enum dag_run_status;
        switch (status)
        {
            case none:    return dag_run_status_names[0];
            case success: return dag_run_status_names[1];
            case running: return dag_run_status_names[2];
            case failed:  return dag_run_status_names[3];
            case skipped: return dag_run_status_names[4];
            case queued:  return dag_run_status_names[5];
            default:      return dag_run_status_names[0];
        }
    }

    auto dag_run_status_from_string(std::string_view value) noexcept -> dag_run_status
    {
        auto normalized = normalize_token(value);
        if (normalized == dag_run_status_names[1]) return dag_run_status::success;
        if (normalized == dag_run_status_names[2]) return dag_run_status::running;
        if (normalized == dag_run_status_names[3]) return dag_run_status::failed;
        if (normalized == dag_run_status_names[4]) return dag_run_status::skipped;
        if (normalized == dag_run_status_names[5]) return dag_run_status::queued;
        return dag_run_status::none;
    }

    [[nodiscard]] static auto metadata_to_json(task_metadata const& metadata) -> nlohmann::json
    {
        nlohmann::json meta;
        meta["schedule"] = {
            {"cron", metadata.schedule.cron_expression},
            {"frequency", to_string(metadata.schedule.frequency)}
        };
        meta["statuses"] = {
            {"previous", to_string(metadata.statuses.previous)},
            {"current", to_string(metadata.statuses.current)}
        };
        return meta;
    }

    [[nodiscard]] static auto metadata_from_json(nlohmann::json const& json) -> std::optional<task_metadata>
    {
        if (not json.is_object())
            return std::nullopt;

        task_metadata metadata{};
        bool has_data = false;

        if (auto const it = json.find("schedule"); it != json.end() and it->is_object())
        {
            metadata.schedule.cron_expression = it->value("cron", "");
            metadata.schedule.frequency = schedule_frequency_from_string(it->value("frequency", "custom"));
            has_data = true;
        }

        if (auto const it = json.find("statuses"); it != json.end() and it->is_object())
        {
            metadata.statuses.previous = dag_run_status_from_string(it->value("previous", "none"));
            metadata.statuses.current = dag_run_status_from_string(it->value("current", "none"));
            has_data = true;
        }

        if (not has_data)
            return std::nullopt;

        return metadata;
    }

    // Very simple non-cryptographic hex hash (FNV-1a 64-bit) for IDs
    static std::string fnv1a_hex(const std::span<const std::byte> bytes)
    {
        uint64_t hash = 1469598103934665603ull;
        for (auto b: bytes)
        {
            hash ^= static_cast<uint8_t>(b);
            hash *= 1099511628211ull;
        }
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << hash;
        return oss.str();
    }

    // ---------- fs_blob_store ----------
    fs_blob_store::fs_blob_store(fs::path root) :
        root_{std::move(root)}, blobs_dir_{root_ / "blobs"}
    {
        std::error_code ec;
        fs::create_directories(blobs_dir_, ec);
    }

    auto fs_blob_store::put_blob(const std::span<const std::byte> bytes) const -> std::expected<std::string, storage_error>
    {
        auto id = fnv1a_hex(bytes);
        const fs::path p = blobs_dir_ / (id + ".bin");
        if (std::error_code ec; not fs::exists(p, ec))
        {
            std::ofstream os(p, std::ios::binary);
            if (not os)
                return std::unexpected(storage_error::io);
            os.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (not os)
                return std::unexpected(storage_error::io);
        }
        return id;
    }

    auto fs_blob_store::get_blob(const std::string_view id) const -> std::expected<std::vector<std::byte>, storage_error>
    {
        const fs::path p = blobs_dir_ / (std::string{id} + ".bin");
        std::ifstream is(p, std::ios::binary);
        if (not is)
            return std::unexpected(storage_error::not_found);
        const std::vector buf((std::istreambuf_iterator(is)), {});
        if (not is and not is.eof())
            return std::unexpected(storage_error::io);
        std::vector<std::byte> out(buf.size());
        std::memcpy(out.data(), buf.data(), buf.size());
        return out;
    }

    auto fs_blob_store::has_blob(const std::string_view id) const -> std::expected<bool, storage_error>
    {
        const fs::path p = blobs_dir_ / (std::string{id} + ".bin");
        std::error_code ec;
        return fs::exists(p, ec);
    }

    // Helpers for metadata IO
    static auto read_json(fs::path const &p) -> std::expected<nlohmann::json, storage_error>
    {
        std::ifstream is(p);
        if (not is)
            return std::unexpected(storage_error::not_found);
        try
        {
            nlohmann::json j;
            is >> j;
            return j;
        }
        catch (...) { return std::unexpected(storage_error::corrupt); }
    }

    static auto write_json_atomic(fs::path const &p, nlohmann::json const &j) -> std::expected<void, storage_error>
    {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
        auto tmp = p;
        tmp += ".tmp";
        {
            std::ofstream os(tmp);
            if (not os)
                return std::unexpected(storage_error::io);
            os << j.dump(2);
            if (not os)
                return std::unexpected(storage_error::io);
        }
        fs::rename(tmp, p, ec);
        if (ec)
            return std::unexpected(storage_error::io);
        return {};
    }

    // ---------- fs_dag_store ----------
    fs_dag_store::fs_dag_store(fs::path root) :
        root_{std::move(root)}, dag_dir_{root_ / "metadata" / "dags"}
    {
        std::error_code ec;
        fs::create_directories(dag_dir_, ec);
    }

    auto fs_dag_store::upsert_dag(shyguy_dag const &dag, const std::optional<uint64_t> if_version) const -> std::expected<uint64_t, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path p = dag_dir_ / dag.name / "dag.json";
        uint64_t new_version = 1;
        if (fs::exists(p))
        {
            auto cur = read_json(p);
            if (not cur)
                return std::unexpected(cur.error());
            const uint64_t ver = cur->value("version", 0);
            if (if_version and ver != *if_version)
                return std::unexpected(storage_error::conflict);
            new_version = ver + 1;
        }
        else
        {
            if (if_version and *if_version != 0)
                return std::unexpected(storage_error::conflict);
        }

        nlohmann::json j;
        j["version"] = new_version;
        j["value"] = dag;
        if (auto w = write_json_atomic(p, j); not w)
            return std::unexpected(w.error());
        return new_version;
    }

    auto fs_dag_store::get_dag(const std::string_view name) const -> std::expected<dag_entry, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path p = dag_dir_ / std::string{name} / "dag.json";
        auto j = read_json(p);
        if (not j)
            return std::unexpected(j.error());
        dag_entry e{};
        e.version = j->value("version", 0);
        e.value = j->at("value").get<shyguy_dag>();
        return e;
    }

    auto fs_dag_store::list_dags() const -> std::expected<std::vector<dag_entry>, storage_error>
    {
        std::scoped_lock lk(mtx_);
        std::vector<dag_entry> out;
        if (std::error_code ec; not fs::exists(dag_dir_, ec))
            return out;

        for (auto const &d: fs::directory_iterator(dag_dir_))
        {
            if (not d.is_directory())
                continue;
            auto p = d.path() / "dag.json";
            auto j = read_json(p);
            if (not j)
                continue;
            dag_entry e{};
            e.version = j->value("version", 0);
            try { e.value = j->at("value").get<shyguy_dag>(); }
            catch (...) { continue; }
            out.push_back(std::move(e));
        }
        return out;
    }


    auto fs_dag_store::erase_dag(const std::string_view name, const std::optional<uint64_t> if_version) const -> std::expected<void, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path dir = dag_dir_ / std::string{name};
        if (const fs::path p = dir / "dag.json"; fs::exists(p))
        {
            if (if_version)
            {
                auto j = read_json(p);
                if (not j)
                    return std::unexpected(j.error());
                if (const uint64_t ver = j->value("version", 0); ver != *if_version)
                    return std::unexpected(storage_error::conflict);
            }
            std::error_code ec;
            fs::remove(p, ec);
            fs::remove(dir, ec);
            return {};
        }
        return std::unexpected(storage_error::not_found);
    }

    // ---------- fs_task_store ----------
    fs_task_store::fs_task_store(fs::path root) :
        root_{std::move(root)}, task_dir_{root_ / "metadata" / "tasks"}
    {
        std::error_code ec;
        fs::create_directories(task_dir_, ec);
    }


   auto fs_task_store::upsert_task(
    shyguy_task const &task,
    const std::optional<std::string> &blob_id,
    const std::optional<uint64_t> if_version,
    const std::optional<task_metadata> &metadata) const -> std::expected<uint64_t, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path dir = task_dir_ / task.associated_dag;
        const fs::path p = dir / (task.name + ".json");
        uint64_t new_version = 1;
        std::optional<nlohmann::json> existing_metadata;
        if (fs::exists(p))
        {
            auto cur = read_json(p);
            if (not cur)
                return std::unexpected(cur.error());
            const uint64_t ver = cur->value("version", 0);
            if (if_version and ver != *if_version)
                return std::unexpected(storage_error::conflict);
            new_version = ver + 1;
            if (cur->contains("metadata"))
                existing_metadata = cur->at("metadata");
        }
        else
        {
            if (if_version and *if_version != 0)
                return std::unexpected(storage_error::conflict);
        }
        nlohmann::json j;
        j["version"] = new_version;
        j["value"] = task;
        if (blob_id)
            j["blob_id"] = *blob_id;
        if (metadata)
            j["metadata"] = metadata_to_json(*metadata);
        else if (existing_metadata)
            j["metadata"] = *existing_metadata;
        if (auto w = write_json_atomic(p, j); not w)
            return std::unexpected(w.error());
        return new_version;
    }


    auto fs_task_store::get_task(const std::string_view dag, const std::string_view name) const -> std::expected<task_entry, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path p = task_dir_ / std::string{dag} / (std::string{name} + ".json");
        auto j = read_json(p);
        if (not j)
            return std::unexpected(j.error());
        task_entry e{};
        e.version = j->value("version", 0);
        e.value = j->at("value").get<shyguy_task>();
        if (j->contains("blob_id"))
            e.blob_id = j->value("blob_id", "");
        if (j->contains("metadata"))
            e.metadata = metadata_from_json(j->at("metadata"));
        return e;
    }

    auto fs_task_store::list_tasks(const std::string_view dag) const -> std::expected<std::vector<task_entry>, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path dir = task_dir_ / std::string{dag};
        std::vector<task_entry> out;
        if (std::error_code ec; not fs::exists(dir, ec))
            return out;
        for (auto const &f: fs::directory_iterator(dir))
        {
            if (not f.is_regular_file())
                continue;
            auto j = read_json(f.path());
            if (not j)
                continue;
            task_entry e{};
            e.version = j->value("version", 0);
            try { e.value = j->at("value").get<shyguy_task>(); }
            catch (...) { continue; }
            if (j->contains("blob_id"))
                e.blob_id = j->value("blob_id", "");
            if (j->contains("metadata"))
                e.metadata = metadata_from_json(j->at("metadata"));
            out.push_back(std::move(e));
        }
        return out;
    }


    auto fs_task_store::erase_task(const std::string_view dag, const std::string_view name, const std::optional<uint64_t> if_version) const -> std::expected<void, storage_error>
    {
        std::scoped_lock lk(mtx_);
        const fs::path p = task_dir_ / std::string{dag} / (std::string{name} + ".json");
        if (not fs::exists(p))
            return std::unexpected(storage_error::not_found);
        if (if_version)
        {
            auto j = read_json(p);
            if (not j)
                return std::unexpected(j.error());
            if (const uint64_t ver = j->value("version", 0); ver != *if_version)
                return std::unexpected(storage_error::conflict);
        }
        std::error_code ec;
        fs::remove(p, ec);
        if (ec)
            return std::unexpected(storage_error::io);
        return {};
    }

    // ---------- fs_storage ----------
    fs_storage::fs_storage(const fs::path& root) :
        root_{root}, blobs_{root}, dags_{root}, tasks_{root}
    {
    }

} // namespace cosmos::inline v1
