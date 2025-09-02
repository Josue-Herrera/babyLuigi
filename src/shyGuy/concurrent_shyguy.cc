
#include "concurrent_shyguy.hpp"
#include "process/system_execution.hpp"

#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>

#include <ranges>
#include <fstream>

namespace cosmos::inline v1
{
    auto get_date()
    {
        // Get current date
        auto const now   = std::chrono::system_clock::now();
        auto const today = std::chrono::floor<std::chrono::days>(now);
        std::chrono::year_month_day ymd {std::chrono::sys_days{today}};
        // Format: year_month_day
        std::string date_folder = std::format(
            "{:%Y_%m_%d}",
            ymd
        );

        return date_folder;
    }

    auto prefix_folder()
    {
#ifdef _WIN32
        if (auto const app_data = std::getenv("APPDATA"))
            return std::filesystem::path(app_data);
        else 
            return std::filesystem::path("C:/ProgramData");
#else
        auto app_data = std::getenv("$COSMOS_HOME");
        return std::filesystem::path(app_data);
#endif
    }

    auto concurrent_shyguy::process(shyguy_request const &request) noexcept -> command_result_type
    {
        std::lock_guard lock(mutex);
        return request.data | match
        {
            [this, command = request.command](requestable auto const& dag_or_task) -> command_result_type
            {
                switch (command)
                {
                    case command_enum::create: return create(dag_or_task);
                    case command_enum::remove: return remove(dag_or_task);
                    case command_enum::execute: return execute(dag_or_task);
                    case command_enum::snapshot: return snapshot(dag_or_task);
                    default: return { std::unexpected(command_error::unknown_command) };
                }
            },
            [this](std::monostate const m) { return process(m); }
        };
    }

    auto concurrent_shyguy::create(shyguy_dag const &dag) noexcept -> command_result_type
    {
        if (auto const dag_found = dags.find(dag.name); dag_found != dags.end())
            return std::unexpected(command_error::dag_duplicate);

        if (auto [_, in] = dags.emplace(dag.name, directed_acyclic_graph(dag.name)); not in)
            return std::unexpected(command_error::dag_insertion_failed);

        if (has_schedule(dag))
            schedules.emplace(dag.name, dag.schedule.value());

        auto const content_folder = create_content_folder("dags/" + dag.name);
        if (auto const content_json = create_content_json(content_folder, dag); content_json.empty())
        {
            logger->warn("Failed to create content.json for dag {}", dag.name);
            return std::unexpected(command_error::dag_insertion_failed);
        }

        return log_return("created dag {}", dag.name);
    }

    auto concurrent_shyguy::remove(shyguy_dag const &dag) noexcept -> command_result_type
    {
        if (bool const erased = dags.erase(dag.name); erased)
        {
            if (has_schedule(dag))
                schedules.erase(dag.name);

           return log_return("removed dag {}", dag.name);
        }

        return std::unexpected(command_error::dag_deletion_failed);
    }

    auto concurrent_shyguy::execute(shyguy_dag const &dag) noexcept -> command_result_type
    {
        auto const run_folder = create_run_folder("dags/" + dag.name);
        auto const dag_iter   = dags.find(dag.name);
        if (dag_iter == end(dags))
            return std::unexpected(command_error::dag_not_found);

        auto const ordered_tasks = dag_iter->second.run_order();

        if (not ordered_tasks)
            return std::unexpected(command_error::task_creates_cycle);

        auto const& order = ordered_tasks.value();

        request_queue->enqueue
        ({
            std::views::transform(order, [this](auto const& task_name)
            {
                task_runner runner{};
                runner.name = task_name;
                runner.contents = find_content("tasks/" + task_name);
                runner.task_function = [this] () noexcept -> void
                {
                    std::lock_guard lock (mutex);
                    std::string const command {"./task_executable"}; // Placeholder for actual command
                    if (auto output = execute_command(command); output)
                        logger->info("Task executed successfully with output: {}", output.value());
                    else
                        logger->error("Task execution failed with exit code: {}", output.error());
                };
                return runner;
            }) | std::ranges::to<std::vector>(),

            dag_iter->second
            }
        );

        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::snapshot(shyguy_dag const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::create(shyguy_task const &task) noexcept -> command_result_type
    {
        task_map.insert_or_assign(task.name, task);
        auto &dag = dags.at(task.associated_dag);
        if (auto const inserted = dag.push_task(task.name, task.dependency_names); not inserted)
            return std::unexpected(static_cast<command_error>(inserted.error()));

        auto const content_folder = create_content_folder("tasks/" + task.name);
        if(task.file_content and task.filename)
        {
            auto const content_file = content_folder / "content";
            if (std::ofstream file(content_file, std::ios::binary); file.is_open())
            {
                file.write(std::data(*task.file_content), std::size(*task.file_content));
                file.close();
            }
            else
            {
                logger->warn("Failed to create task file at {}", content_file.string());
                return std::unexpected(command_error::task_file_creation_failed);
            }
        }

        return log_return("Created New Task {}", task.name);
    }

    auto concurrent_shyguy::remove(shyguy_task const& task) noexcept -> command_result_type
    {
        if (auto const dag = dags.find(task.associated_dag); dag != std::end(dags))
        {
            if (auto const removed = dag->second.remove_task(task.name); not removed)
                return std::unexpected(static_cast<command_error>(removed.error()));

        }
        return log_return("Removed Task {}", task.name);
    }

    auto concurrent_shyguy::execute(shyguy_task const &task) noexcept -> command_result_type
    {
        using namespace std::string_literals;
        std::lock_guard lock(mutex);
        auto const dag = dags.find(task.associated_dag);
        if (dag == std::end(dags))
            return std::unexpected(command_error::dag_not_found);
        auto const run_folder    = create_run_folder("dags/"s.append(dag->second.view_name()));
        auto const ordered_tasks = dag->second.run_order();
        if (not ordered_tasks)
            return std::unexpected(command_error::task_creates_cycle);

        auto const& order = ordered_tasks.value();


        return std::unexpected(command_error::not_currently_supported);
    }
    auto concurrent_shyguy::snapshot(shyguy_task const &) noexcept -> command_result_type
    {
        return std::unexpected(command_error::not_currently_supported);
    }

    auto concurrent_shyguy::process(std::monostate) const noexcept -> command_result_type
    {
        logger->info("Monostate input. This should never happen.");
        return std::unexpected(command_error::monostate_reached);
    }

    auto concurrent_shyguy::next_scheduled_dag() const noexcept -> std::optional<notification_type>
    {
        std::lock_guard lock(this->mutex);
        return {notification_type{.time = std::chrono::steady_clock::now() + std::chrono::seconds{5},
                                  .associated_request = {},
                                  .command_type = command_enum::execute}};
    }

     auto concurrent_shyguy::create_content_json(std::filesystem::path const& app, shyguy_dag const &request) noexcept -> std::filesystem::path
    {
        // This might be better in a different file. the definition of the content file
        const nlohmann::json content_json
        {
            {"created_at", get_date()},
            {"tasks", nlohmann::json::array()},
            {"id" , uuids::to_string(uuid_generator.generate())},
            {"dag_name", request.name},
            {"dag_schedule", request.schedule.value_or(std::string{})},
        };

        auto content_file = app / "dag_manifest.json";
        if (std::filesystem::exists(content_file))
            return content_file;

        std::ofstream file(content_file);
        if (not file.is_open())
        {
            logger->error("Failed to create content.json at {}", content_file.string());
            return std::filesystem::path{};
        }

        file << content_json.dump(4);
        file.close();
        return content_file;
    }

    auto concurrent_shyguy::create_run_folder(std::filesystem::path const& app) noexcept -> std::filesystem::path
    {
        auto run_folder = prefix_folder() / "cosmos" / "shyguy" / app
            / get_date() / uuids::to_string(uuid_generator.generate());

        if (std::filesystem::exists(run_folder))
            return run_folder;

        std::filesystem::create_directories(run_folder);
        return run_folder;
    }

    auto concurrent_shyguy::find_content(std::filesystem::path const& suffix) noexcept -> std::vector<char>
    {
        auto content = prefix_folder() / "cosmos" / "shyguy" / "content" / suffix / "content";

        // ignore extensions for now, since we don't have it.
        if (not std::filesystem::exists(content))
            return {};

        if (std::ifstream file(content, std::ios::binary); not file.is_open())
            return {};
        else
            return { std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
    }

    auto concurrent_shyguy::create_content_folder(std::filesystem::path const& app) noexcept -> std::filesystem::path
    {
        auto run_folder = prefix_folder() / "cosmos" / "shyguy" / "content" / app;

        if (std::filesystem::exists(run_folder))
            return run_folder;

        std::filesystem::create_directories(run_folder);
        return run_folder;
    }
}


