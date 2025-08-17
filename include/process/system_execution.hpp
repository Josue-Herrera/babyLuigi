#pragma once

// *** Standard Includes ***
#include <array>
#include <expected>
#include <ostream>
#include <string>

#ifdef _WIN32
#include <stdio.h>
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif


namespace cosmos::inline v1
{

    using exitstatus_t = int;
    /**
     * @brief system command and get STDOUT result.
     *
     * @param command system command to execute
     *
     * @return commandResult containing STDOUT (not stderr) output & exitstatus
     * of command. Empty if command failed (or has no output).
     *
     * @note If you want stderr, use shell redirection (2&>1).
     */
    inline auto execute_command(std::string const& command) noexcept -> std::expected<std::string, exitstatus_t>
    {
        std::array<char, 10000> buffer{};

        std::FILE* pipe = popen(command.c_str(), "r");

        if (pipe == nullptr)
        {
            return std::unexpected(EXIT_FAILURE);
        }

        std::string result{};
        while (true)
        {
            const std::size_t size = std::fread(buffer.data(), sizeof(char), sizeof(buffer), pipe);
            if (not size)
            {
                break;
            }

            result += std::string(buffer.data(), size);
        }

        const auto status = pclose(pipe);
        if (auto exitcode = WEXITSTATUS(status); exitcode != EXIT_SUCCESS && result.empty())
        {
            return std::unexpected(exitcode);
        }

        return { result };
    }
} // namespace cosmos::inline v1

