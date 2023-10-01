#pragma once 

// *** 3rd Party Includes ***
#include <asio.hpp>
#include <spdlog/spdlog.h>

// *** Standard Includes ***
#include <vector>
#include <string>
#include <thread>
#include <unordered_map>    
#include <queue>
#include <mutex>
#include <condition_variable>

// *** Project Includes ***
#include <task_system/task_system.hpp>
#include <task.hpp>

namespace jx {
inline namespace v1 {
    

    template <class T>
    class SafeQueue
    {
    public:
        SafeQueue(void)
            : q(), m(), c() {}

        ~SafeQueue(void){}

        // Add an element to the queue.
        void enqueue(T t)
        {
            std::lock_guard<std::mutex> lock(m);
            q.push(t);
            c.notify_one();
        }

        // Get the "front"-element.
        // If the queue is empty, wait till a element is avaiable.
        T dequeue(void)
        {
            std::unique_lock<std::mutex> lock(m);
            while(q.empty())
            {
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
            }
            T val = q.front();
            q.pop();
            return val;
        }

        private:
        std::queue<T> q;
        mutable std::mutex m;
        std::condition_variable c;
    };

    class task_graph
    {
        using task_dependencies = std::unordered_multimap<std::string, std::string>;
        task_dependencies dependencies{};
    public:

    };

    enum class respose_reason { not_set, revoking , completing };

    struct task_view
    {
        std::string_view request{};
        
        respose_reason response {};

        std::shared_ptr<SafeQueue<respose_reason>> response_queue {};
    };

    class task_maintainer : task_system
    {
    
    public:
        auto service(task_view) noexcept -> bool;
        
    };

    class shy_guy
    {
    public:

        auto run () noexcept -> void;

        auto stop () noexcept -> void;

        auto wait_for_connection () noexcept -> void;

        auto read_header(asio::ip::tcp::socket&) noexcept -> void;
        auto read_body( asio::ip::tcp::socket&, std::size_t) noexcept -> void;

        shy_guy(unsigned port);

        ~shy_guy();

        shy_guy(shy_guy const&) = delete;

        shy_guy(shy_guy&&) = delete;

        shy_guy& operator=(shy_guy const&) = delete;

        shy_guy& operator=(shy_guy &&) = delete;

    private:

        // context must come first
        asio::io_context ctx_;
        std::thread ctx_thread_;

        // handles new connections 
        asio::ip::tcp::acceptor acceptor_; 

        // task maintainer
        task_maintainer tasks_;
        SafeQueue<task_view> task_queue_{};
        std::thread graph_thread_;

        std::atomic_bool terminate_{};
    };
        
} // namespace v1
} // namespace jx

