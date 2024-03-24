#pragma once

#include <string>
#include <vector>

namespace jx 
{
    inline namespace v1 
    {
        struct task_description
        {
            std::string name{};
            std::string type{};
            std::string file_location{};
            std::vector<std::string> dependency_names{};
            std::vector<char> payload{};
        };

        struct shy_guy_info
        {
            std::string port {"90909"};
            std::string server_address{"127.0.0.1"};
        };

        [[nodiscard]] auto valid(task_description const&) noexcept -> bool;

        [[nodiscard]] auto submit_task_request(task_description const& task,  shy_guy_info const& info) noexcept -> bool;
        
        [[nodiscard]] auto download_contents(std::string const& path) noexcept -> std::vector<char>;

        inline static constexpr auto baby_luigi_banner = "\n\nBABY LUIGI\n";

/*
         "\n\n" \
" ░▒▓███████▓▒░   ░▒▓██████▓▒░  ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░       ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░  ░▒▓██████▓▒░  ░▒▓█▓▒░\n" \
" ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░       ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░\n" \
" ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░       ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░ ░▒▓█▓▒░        ░▒▓█▓▒░\n" \
" ░▒▓███████▓▒░  ░▒▓████████▓▒░ ░▒▓███████▓▒░   ░▒▓██████▓▒░        ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░ ░▒▓█▓▒▒▓███▓▒░ ░▒▓█▓▒░\n" \
" ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░    ░▒▓█▓▒░           ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░\n" \
" ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░    ░▒▓█▓▒░           ░▒▓█▓▒░        ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░ ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓█▓▒░\n" \
" ░▒▓███████▓▒░  ░▒▓█▓▒░░▒▓█▓▒░ ░▒▓███████▓▒░     ░▒▓█▓▒░           ░▒▓████████▓▒░  ░▒▓██████▓▒░  ░▒▓█▓▒░  ░▒▓██████▓▒░  ░▒▓█▓▒░\n";
*/
    }
}
