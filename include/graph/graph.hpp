
#pragma once

#include <vector>
#include <unordered_map>

namespace jx 
{
    inline namespace v1 
    {
        // This Graph will use an adjacency list.
        template<class T, class U = T>
        class graph
        {    
            using adjacencyList = std::unordered_multimap<T, U>;

        
        private:
            adjacencyList adjacency_list{};

        };
    }
} // namespace jx
