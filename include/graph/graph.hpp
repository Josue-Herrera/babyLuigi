
#pragma once

#include <vector>
#include <unordered_map>

namespace jx 
{
    inline namespace v1 
    {
        // This Graph will use an adjacency list.
        template<class T>
        class graph
        {    
            using AdjacencyList = std::unordered_multimap<T, T>;

            
        
        private:
            AdjacencyList dependency_list{};
        };
    }
} // namespace jx
