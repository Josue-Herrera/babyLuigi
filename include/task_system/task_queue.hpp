#pragma once

#include <queue>
#include <mutex>
#include <optional>

namespace jx { 

    inline namespace v1 { 

        template <class T> 
        class blocking_queue {
        public:
          // Add an element to the queue.
          void enqueue(T t) {
            std::lock_guard<std::mutex> lock(m);
            q.push(t);
            c.notify_one();
          }

          // Get the "front"-element.
          // If the queue is empty, wait till a element is avaiable.
          T dequeue(void) {
            std::unique_lock<std::mutex> lock(m);
            while (q.empty()) {
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


        template <class T>
        class nonblocking_queue {
        public:

            // Add an element to the queue.
            void enqueue(T t) {
                std::lock_guard<std::mutex> lock(m);
                q.push(t);
            }

            // Get the "front"-element.
            std::optional<T> dequeue(void) {
                std::unique_lock<std::mutex> lock(m);
                if (q.empty()) {
                    return std::nullopt;
                }
                T val = std::move(q.front());
                q.pop();
                return std::optional<T> { std::in_place_t{}, val };
            }

        private:
            std::queue<T> q;
            mutable std::mutex m;
            std::condition_variable c;
        };
    }
}
