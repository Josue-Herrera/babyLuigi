#pragma once

#include "shyguy_request.hpp"
#include "define_arguements.hpp"
#include <string>
#include <vector>

namespace cosmos
{
    inline namespace v1
    {



        [[nodiscard]] auto valid(shyguy_request const&) noexcept -> bool;

        [[nodiscard]] auto submit_task_request(shyguy_request const& task,  shy_guy_info const& info) noexcept -> bool;

        [[nodiscard]] auto download_contents(std::string const& path) noexcept -> std::string;

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

namespace cosmos
{
    // High-level entry point to parse CLI, build a request, validate and submit it.
    auto run_cli_and_submit(int argc, const char** argv) -> int;
}
