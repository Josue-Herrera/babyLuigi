

#include <filesystem>
#include <baby_luigi.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace jx
{
    inline namespace v1
    {
        using json = nlohmann::json;

        auto submit_task_request(task_description const& task,  shy_guy_info const& info) noexcept -> bool 
        {
            return true;
        }

        auto valid(task_description const& task) noexcept -> bool
        {
            std::filesystem::path file_path(task.file_location);
            if (not std::filesystem::exists(file_path))
            {
                spdlog::error("File does not exist: {}", task.file_location);
                return false;
            }

            if(not std::filesystem::is_regular_file(file_path))
            {
                spdlog::error("This is not a regular file: {}", task.file_location);
                return false;
            }

            return true;
        }

        auto download_contents(std::string const& path) noexcept -> std::vector<char>
        {
            if (std::ifstream source_file { path, std::ios::binary }; source_file)
            {
                return std::vector<char>(std::istreambuf_iterator<char>{source_file}, {});
            }

            spdlog::error("Unable to correctly open file: {} .", path);

            return {};
        }
    } // namespace v1    
} // namespace jx
