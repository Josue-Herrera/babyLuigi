#pragma once 

// *** 3rd Party Includes ***
#include <asio.hpp>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <vector>
#include <thread>

namespace jx {
inline namespace v1 {

    class connection : std::enable_shared_from_this<connection>
    {
    public:
    
        [[nodiscard]] auto is_connected() noexcept -> bool;
        auto disconnect() noexcept -> void;
    };
    
    class shy_guy
    {
    public:

        shy_guy(unsigned port);

        auto run () noexcept -> void;

        auto stop () noexcept -> void;

        auto wait_for_connection () noexcept -> void;

    private:

        std::vector<connection> connections_;

        // context must come first
        asio::io_context ctx_;
        std::thread ctx_thread_;

        // handles new connections 
        asio::ip::tcp::acceptor acceptor_; 
    };
        
} // namespace v1
} // namespace jx

