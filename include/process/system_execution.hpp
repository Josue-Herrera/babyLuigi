#pragma once

#include <array>
#include <ostream>
#include <string>

#ifdef _WIN32
#include <stdio.h>
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif

namespace jx 
{

    class system_result
    {
    public:

        friend std::ostream &operator<<(std::ostream &os, system_result const& result) 
        {
            os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
            return os;
        }

        std::string output;
        int exitstatus;
    };

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
     inline auto execute(std::string const& command) noexcept -> system_result
     {
        std::array<char, 1048576> buffer {};
      
        std::FILE * pipe = popen(command.c_str(), "r");

        if (pipe == nullptr)
        {
            return system_result{ std::string("[error] unable to run command."), EXIT_FAILURE };
        }
      
        std::string result{};
        while (true)
        {
            std::size_t bytesread = std::fread(buffer.data(), sizeof(char), sizeof(buffer), pipe); 

            if (not bytesread)
            {
                break;
            }
                
            result += std::string(buffer.data(), bytesread);
        }

        auto status   = pclose(pipe);
        auto exitcode = WEXITSTATUS(status);
        return system_result{ result, exitcode };
    }



} // namespace jx