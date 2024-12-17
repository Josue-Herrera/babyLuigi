
#pragma once

#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <range/v3/algorithm/find_first_of.hpp>


namespace cosmos::inline v1
{

        // This Graph will use an adjacency list.
        template<class T, class U = T>
        class graph
        {    
            using adjacencyList = std::unordered_multimap<T, U>;

        
        private:
            adjacencyList adjacency_list{};

        };


        enum class graph_color { white, gray, black };

        enum class graph_error { no_error, dependencies_not_found, constains_duplicates, task_creates_cycle };
        
        constexpr auto check(graph_error error) noexcept -> bool {
            return error != graph_error::no_error;
        }

        [[nodiscard]] inline auto to_cstring(graph_error const error) noexcept {
            switch (error) {
            case graph_error::no_error: return "no error";
            case graph_error::dependencies_not_found: return "dependencies not found";
            case graph_error::constains_duplicates: return "constains duplicates";
            case graph_error::task_creates_cycle: return "task creates cycle";
            default: return "unknown";
            }
        }

       
        // This Graph will use an adjacency list.
        class directed_acyclic_graph
        {
            using adjacency_list_type = std::unordered_map<std::string, std::vector<std::string>>;
            using color_map_type      = std::unordered_map<std::string, graph_color>;
        public:
            directed_acyclic_graph() = default;
            directed_acyclic_graph(directed_acyclic_graph const&) = default;
            directed_acyclic_graph(directed_acyclic_graph&&) = default;
            
            // If created with a task it means its the root.
            explicit  directed_acyclic_graph(std::string name) noexcept :
                root_name_{std::move(name)}
            {
                adjacency_list.emplace(root_name_, std::vector<std::string>{});
            }

            auto root_view() const noexcept -> std::string_view {
                return root_name_;
            }
            auto dependencies_of(std::string const& name) const -> auto const& { 
                return adjacency_list.at(name);   
            }

            inline auto push_task(
                std::string const& task_name,
                std::optional<std::vector<std::string>> const& dependencies
            ) noexcept {
                if (not dependencies)
                    return graph_error::dependencies_not_found;

                // checking for duplicate dependancies
                if (adjacency_list.contains(task_name)) {
                    auto const  range      = adjacency_list.at(task_name);
                    auto const& depends    = dependencies.value();

                    if (ranges::find_first_of(range,  depends) != std::end(range))
                        return graph_error::constains_duplicates;
                }
              
                adjacency_list.emplace(task_name, dependencies.value());
                
                if (has_cycle()) {
                    adjacency_list.erase(task_name);
                    return graph_error::task_creates_cycle;
                }

                return graph_error::no_error;
            }

            // inline auto remove_task ()

        private:


            /// <summary>
            /// This function computes the run order of a given set of task and their dependencies; 
            /// </summary>
            /// <returns>idk</returns>
            auto compute_run_order() {

            }

            inline bool depth_first_search(color_map_type& visited, std::string const& node) const {
                visited[node] = graph_color::gray;
               
                for (auto const& neighbor : adjacency_list.at(node)) {
                    if (visited[neighbor] == graph_color::gray) {
                        return true;
                    }
                    else if (visited[neighbor] == graph_color::white) {
                        if (depth_first_search(visited, neighbor)) {
                            return true;
                        }
                    }
                }

                visited[node] = graph_color::black;
                return false;
            }

            inline auto has_cycle() const -> bool {
                color_map_type visited{};

                for (const auto& entry : adjacency_list) {
                    visited.emplace(entry.first, graph_color::white);
                }

                for (const auto& entry : adjacency_list) {
                    if (visited[entry.first] == graph_color::white) {
                        if (depth_first_search(visited, entry.first)) {
                            return true;
                        }
                    }
                }

                return false;
            }

            auto find_valid_root_task(std::string const& request) {

                //for (auto& [root, tasks] : root_dependancies) {

                //    auto found{ ranges::find_first_of(tasks, request.dependency_names.value()) };

                //    if (found != tasks.end()) {

                //        if (has_cycle(root, request.name))
                //            return tl::unexpected(graph_error::task_creates_cycle);

                //        if (ranges::contains(tasks, request.name)) // task already exists
                //            return tl::unexpected(graph_error::constains_duplicates);

                //        return { root };
                //    }
                //}

                return request.size();
            }


            std::string         root_name_{};
            adjacency_list_type adjacency_list{};
        };

} // namespace cosmos::v1
