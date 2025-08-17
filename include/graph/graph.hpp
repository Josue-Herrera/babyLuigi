#pragma once

#include <expected>
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <range/v3/algorithm/find_first_of.hpp>


namespace cosmos::inline v1
{
    // This Graph will use an adjacency list.
    template <class T, class U = T>
    class graph
    {
        using adjacencyList = std::unordered_multimap<T, U>;

    private:
        adjacencyList adjacency_list{};
    };

    enum class graph_color { white, gray, black };
    enum class graph_error
    {
        dependencies_not_found,
        contains_duplicates,
        task_creates_cycle,
        task_not_found
    };

    enum class visit_state { not_visited, visiting, visited };

    [[nodiscard]] inline auto to_cstring(graph_error const error) noexcept
    {
        switch (error)
        {
        case graph_error::dependencies_not_found: return "dependencies not found";
        case graph_error::contains_duplicates: return "contains duplicates";
        case graph_error::task_creates_cycle: return "task creates cycle";
        case graph_error::task_not_found: return "task not found";
        default: return "unknown";
        }
    }

    // This Graph will use an adjacency list.
    class directed_acyclic_graph
    {
        using root_name_str = std::string;
        using name_str      = std::string;
        using adjacency_list_type = std::unordered_map<root_name_str, std::vector<name_str>>;
        using color_map_type = std::unordered_map<name_str, graph_color>;

    public:
        directed_acyclic_graph() = default;
        directed_acyclic_graph(directed_acyclic_graph const&) = default;
        directed_acyclic_graph(directed_acyclic_graph&&) noexcept = default;

        explicit directed_acyclic_graph(std::string name) noexcept :
            root_name{std::move(name)}
        {
            adjacency_list.emplace(root_name, std::vector<std::string>{});
        }

        [[nodiscard]] auto view_name() const noexcept -> std::string_view
        {
            return root_name;
        }

        [[nodiscard]] auto dependencies_of(std::string const& name) const -> auto const&
        {
            return adjacency_list.at(name);
        }

        [[nodiscard]] auto contains(std::string const& task_name) const -> bool
        {
            return adjacency_list.contains(task_name);
        }

        inline auto push_task(
            name_str const& task_name,
            std::optional<std::vector<name_str>> const& dependencies
        ) noexcept -> std::expected<adjacency_list_type::iterator, graph_error>
        {

            if (contains(task_name))
                return std::unexpected(graph_error::contains_duplicates);

            auto&& depends = dependencies.value_or(std::vector<name_str>{});
            auto [iter, inserted] = adjacency_list.emplace(task_name, depends);
            if (not inserted)
                return std::unexpected(graph_error::contains_duplicates);

            if (has_cycle())
            {
                adjacency_list.erase(task_name);
                return std::unexpected(graph_error::task_creates_cycle);
            }

            return { iter };
        }

        inline auto remove_task (name_str const& task_name) -> std::expected<std::monostate, graph_error>
        {
            auto const task = adjacency_list.find(task_name);
            if (task == adjacency_list.end())
                return std::unexpected(graph_error::task_not_found);
            
            for(auto& dependencies : adjacency_list | std::views::values)
            {
                auto found = std::ranges::find(dependencies, task_name);
                if (found == dependencies.end())
                    continue;

                dependencies.erase(found);
            }

            adjacency_list.erase(task);
            return {};
        }

        [[nodiscard]] auto run_order() const
        {
            return topological_sort();
        }

        [[nodiscard]] auto topological_sort() const -> std::optional<std::vector<std::string>>
        {
            std::unordered_map<std::string, visit_state> state;
            std::vector<std::string> order;
            bool has_cycle = false;

            for (const auto& name : adjacency_list | std::views::keys)
            {
                if (state[name] == visit_state::not_visited)
                {
                    dfs(state, has_cycle, order, name);
                    if (has_cycle)
                        return std::nullopt;
                }
            }

            std::ranges::reverse(order);
            return order;
        }


    private:

        auto dfs(auto& state, auto& has_cycle, auto& order, const std::string& node) const -> void
        {
            if (state[node] == visit_state::visiting) {
                has_cycle = true;
                return;
            }
            if (state[node] == visit_state::visited)
                return;
            state[node] = visit_state::visiting;
            for (const auto& dep : adjacency_list.at(node)) {
                dfs(state, has_cycle, order, dep);
                if (has_cycle) return;
            }
            state[node] = visit_state::visited;
            order.push_back(node);
        };

        inline auto depth_first_search(color_map_type& visited, std::string const& node) const -> bool
        {
            visited[node] = graph_color::gray;

            for (auto const& neighbor : adjacency_list.at(node))
            {
                if (visited[neighbor] == graph_color::gray)
                {
                    return true;
                }
                else if (visited[neighbor] == graph_color::white)
                {
                    if (depth_first_search(visited, neighbor))
                    {
                        return true;
                    }
                }
            }

            visited[node] = graph_color::black;
            return false;
        }

        [[nodiscard]] inline auto has_cycle() const -> bool
        {
            color_map_type visited{};

            for (const auto& key : adjacency_list | std::views::keys)
            {
                visited.emplace(key, graph_color::white);
            }

            for (const auto& key : adjacency_list | std::views::keys)
            {
                if (visited[key] == graph_color::white)
                {
                    if (depth_first_search(visited, key))
                    {
                        return true;
                    }
                }
            }

            return false;
        }


        root_name_str root_name{};
        adjacency_list_type adjacency_list{};
    };
} // namespace cosmos::v1
