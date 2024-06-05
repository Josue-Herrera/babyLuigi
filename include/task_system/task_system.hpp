#pragma once 

// *** Task Stealing System ***
#include <atomic>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace jx 
{

// From Sean Parent (https://youtu.be/zULU6Hhp42w?si=VgR3ZLi5AJuaQ8kR)

// *** Common type vocabulary *** //
using function_signiture_t = auto (void) -> void;
using function_capture_t   = std::function<function_signiture_t>;

class notification_queue
{
	// *** notification_queue type vocabulary *** //
	using queue_t              = std::deque<function_capture_t>;
	using mutex_t              = std::mutex;
	using lock_t			   = std::unique_lock<mutex_t>;
	using cond_var_t           = std::condition_variable;
	
	cond_var_t      ready;
	queue_t         queue;
	mutex_t         mutex;
	bool            finished{ false };

public: 

	notification_queue() = default;
	notification_queue(notification_queue const&) = delete;

	// Attempt to pop something but if the qu is empty or if its busy it will return false.
	[[nodiscard]] auto try_pop(function_capture_t& func) -> bool
    {
		lock_t lock{ mutex, std::try_to_lock };

		if (not lock or queue.empty()) 
            return false;

		func = std::move(queue.front());

		queue.pop_front();

		return true;
	}

	// try_push 
	template<class Function>
	[[nodiscard]] auto try_push(Function && func) -> bool
    {	
        { 
			lock_t lock{ mutex, std::try_to_lock };
			
            if (not lock) 
                return false;

			queue.emplace_back(std::forward<Function>(func));
		}

		ready.notify_one();
		return true;
	}

	auto done() -> void
    {
		{
			lock_t lock{ mutex };
			finished = true;
		}
		ready.notify_all();
	}

	auto pop(function_capture_t& func) -> bool
    {
		lock_t lock { mutex };

		while (queue.empty() and not finished)
            ready.wait(lock);

		if (queue.empty())
            return false;

		func = std::move(queue.front());

		queue.pop_front();

		return true;
	}

	template<class Function>
	auto push(Function&& func) -> void
    {
		lock_t lock{ mutex };
		queue.emplace_back(std::forward<Function>(func));
	}
};

class task_system 
{
	// *** task_system type vocabulary *** //
	using thread_container_t = std::vector<std::thread>;
	using notifications_t    = std::vector<notification_queue>;
	using atomic_index_t     = std::atomic<unsigned>;

	const unsigned int		count{ std::thread::hardware_concurrency() };
	thread_container_t		threads{ };
	notifications_t		    notifications{ count };
	atomic_index_t			index{ 0 };
	unsigned int            k_bound{ 48 };

	auto run(unsigned i) -> void
    {
		while (true)
        {
			function_capture_t func;

			for (unsigned n = 0; n != count; ++n) 
				if (notifications[(i + n) % count].try_pop(func)) 
                    break;

			if (not func and not notifications[i].pop(func))
                break;

			func();
		}
	} 


public:
	task_system() = default; 
	task_system(task_system&&) = default;
	task_system(task_system const&) = delete;
	
	explicit task_system (unsigned k = 48)
		: k_bound{ k } 
    {
		for (unsigned n = 0; n != count; ++n) 
			threads.emplace_back([&, n]{ run(n); });
	}

	~task_system()
    {
		for (auto& ns : notifications) ns.done();
		for (auto& ts : threads)       ts.join();
	}
	
	template<class Function>
	auto async(Function&& work) -> void
    {	
        auto i = index++;

		for (unsigned n = 0; n != count * k_bound; ++n) 
        {
			if(notifications[(i + n) % count].try_push(std::forward<Function>(work)))
                return;
		}

		notifications[i % count].push(std::forward<Function>(work));
	}
};

}
