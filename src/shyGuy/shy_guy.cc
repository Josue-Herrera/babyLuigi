
#include <shy_guy.hpp>

namespace jx {
inline namespace v1 {


    auto shy_guy::run() noexcept -> void
    {
        spdlog::info("shy guy running.");
        
        try 
        {
            wait_for_connection();

            ctx_thread_ = std::thread{[this]{ ctx_.run(); }};
        }
        catch(std::exception& ex)
        {
            spdlog::error("Unhandled exception in s: {}", ex.what());
        }
    }

    auto shy_guy::stop() noexcept -> void
    {
        ctx_.stop();

        if(ctx_thread_.joinable()) ctx_thread_.join();

        spdlog::info("shy guy stopping.");
    }

    auto shy_guy::wait_for_connection() noexcept ->  void 
    {
        acceptor_.async_accept(
            [this](std::error_code error, asio::ip::tcp::socket socket)
            {
                if (error) spdlog::warn("connection error: {}", error.message());
                else 
                {
                    spdlog::info("new connection established {}:{}", 
                        socket.remote_endpoint().address().to_string(),
                        socket.remote_endpoint().port());
                


                }

                wait_for_connection();
            }
        );
    }

    shy_guy::shy_guy(unsigned port)
    : acceptor_(ctx_, 
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), static_cast<uint16_t>(port))) 
    {

    }


} // namespace v1
} // namespace jx

