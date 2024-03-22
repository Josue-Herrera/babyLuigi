#pragma once

#include <string>
#include <vector>
#include <optional>


namespace jx 
{
    inline namespace v1
    {   
        enum class task_type { unset, python3, shell, placeholder, unknown };

        class task
        {
            //uuid
            std::string name{};
            task_type type{};

            std::optional<std::string> file_location{};
            std::optional<std::string> file_contents{};
            std::optional<std::vector<std::string>> dependency_names{};
        };

    } // namespace v1
} // namespace jx
