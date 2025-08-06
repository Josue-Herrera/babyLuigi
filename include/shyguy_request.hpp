#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <type_traits>
#include <variant>
#include <nlohmann/json.hpp>

namespace cosmos::inline v1 {

    template <class... Fs>
    struct match : Fs... {
        using Fs::operator()...;
        constexpr explicit match(Fs &&... fs) : Fs{fs}... {}
    };

    template<class... Ts>
    match(Ts...) -> match<Ts...>;

    template <typename... Ts, typename... Fs>
    constexpr decltype(auto) operator| (std::variant<Ts...> const& v, match<Fs...> const& match) {
        return std::visit(match, v);
    }

    template <typename... Ts, typename... Fs>
    constexpr decltype(auto) operator| (std::variant<Ts...> & v, match<Fs...> const& match) {
        return std::visit(match, v);
    }

    enum class request_type : int { unset, dag, task, unknown };
    enum class task_type { unset, python, shell, unknown };

    inline std::string_view to_string_view(request_type const type) {
        using namespace std::string_view_literals;
        switch (type) {
            case request_type::dag      : return "dag"sv;
            case request_type::task       : return "task"sv;
            case request_type::unset       : return "unset"sv;
            case request_type::unknown     : return "unknown"sv;
            default : return "not-set"sv;
        }
    }

    inline request_type to_request_type(std::string const& str) {
        using namespace std::string_literals;
        if (str ==  "dag"s) return request_type::dag;
        if (str ==  "task"s) return request_type::task;
        if (str ==  "unset"s)  return request_type::unset;
        return request_type::unknown;
    }

    inline std::string_view to_string_view(task_type const type) {
        using namespace std::string_view_literals;
        switch (type) {
            case task_type::python      : return "python"sv;
            case task_type::shell       : return "shell"sv;
            case task_type::unset       : return "unset"sv;
            case task_type::unknown     : return "unknown"sv;
            default : return "not-set"sv;
        }
    }

    inline task_type to_task_type(std::string const& str) {
        using namespace std::string_literals;
        if (str ==  "python"s) return task_type::python;
        if (str ==  "shell"s) return task_type::shell;
        if (str ==  "unset"s)  return task_type::unset;
        return task_type::unknown;
    }
}

template <class T>
struct nlohmann::adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt; // this will call adl_serializer<T>::to_json which will
            // find the free function to_json in T's namespace!
        }
    }

    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt.reset();
        } else {
            opt.emplace(j.get<T>()); // same as above, but with
            // adl_serializer<T>::from_json
        }
    }
};

namespace cosmos::inline v1 {

    struct shyguy_task {
        std::string name{};
        std::string associated_dag{};

        std::optional<std::string> type{};
        std::optional<std::string> filename{};
        std::optional<std::string> file_content{};
        std::optional<std::vector<std::string>> dependency_names{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(shyguy_task,
        name, associated_dag, type, filename, file_content, dependency_names)

    struct shyguy_dag {
        std::string name{};
        std::optional<std::string> schedule{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(shyguy_dag, name, schedule)

    template<class Ts>
    concept requestable = std::same_as<Ts, shyguy_task> or std::same_as<Ts, shyguy_dag>;

    class shyguy_request {
    public:
        using request_t = std::variant<std::monostate, shyguy_dag, shyguy_task>;
        request_t data{std::monostate()};

        auto emplace(auto && value) {
            using underlying_type = std::remove_cvref_t<decltype(value)>;
            using is_dag_type     = std::is_same<underlying_type, shyguy_dag>;
            using is_task_type    = std::is_same<underlying_type, shyguy_task>;
            static_assert(is_dag_type::value ||is_task_type::value);

            data.emplace(value);
        }
    };

    enum class command_enum : uint8_t {
        create,
        remove,
        execute,
        snapshot,
        error
    };

    inline std::string_view to_string_view(command_enum const type) {
        using namespace std::string_view_literals;
        switch (type) {
            case command_enum::create        : return "create"sv;
            case command_enum::remove        : return "remove"sv;
            case command_enum::execute       : return "execute"sv;
            case command_enum::snapshot      : return "snapshot"sv;
            case command_enum::error : return "error"sv;
            default : return "not-set"sv;
        }
    }

    enum class command_error : uint8_t
    {
        dependencies_not_found,
        contains_duplicates,
        task_creates_cycle,
        task_not_found,
        monostate_reached,
        dag_duplicate,
        dag_deletion_failed,
        dag_insertion_failed,
        not_currently_supported,
        dag_not_found,
        dag_not_scheduled,
        dag_schedule_not_found,
        dag_schedule_not_valid,
        unknown_command
    };

    using command_result_type = std::expected<std::string, command_error>;

    struct notification_type {
        std::chrono::steady_clock::time_point time;
        cosmos::shyguy_request associated_request;
        command_enum command_type{};
    };
    struct task_runner {
        cosmos::shyguy_request request;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        command_result_type result;
        std::size_t index{};
    };


    template<class T>
    requires requires(T t) { t.empty(); }
    inline auto exists(std::optional<T> const& val) -> bool {
        return val.has_value() and not val.value().empty();
    }

    inline auto is_dag(shyguy_request const& request) -> bool {
        return std::holds_alternative<shyguy_dag>(request.data);
    }

    inline auto is_task(shyguy_request const& request) -> bool {
        return std::holds_alternative<shyguy_task>(request.data);
    }

    inline auto has_schedule(shyguy_dag const& request) -> bool {
        return exists(request.schedule);
    }

    inline auto has_dependencies(shyguy_request const& request) -> bool {
         if (const auto task = std::get_if<shyguy_task>(&request.data))
            return exists(task->dependency_names);

        return false;
    }

} // namespace cosmos::inline v1


namespace nlohmann {
    template <>
    struct adl_serializer<std::monostate> {
        static void to_json(json& j, const std::monostate&) { j = nullptr; }
        static void from_json(const json&, const std::monostate&) {}
    };

    template <>
    struct adl_serializer<cosmos::shyguy_request> {
        static void to_json(json& j, cosmos::shyguy_request const& request) {
            auto to_json_internal = [&]<typename T0>(T0&& value) {
                const auto request_enum = static_cast<cosmos::request_type>(request.data.index());
                j["type"] = cosmos::to_string_view(request_enum);
                if constexpr (not std::is_same_v<std::monostate, T0>) {
                    j["value"] = std::forward<T0>(value);
                }
            };

            std::visit(to_json_internal, request.data);
        }

        static void from_json(json const& j, cosmos::shyguy_request& request) {
            if (j.is_null()) {
                request.data = std::monostate();
            } else if (j.contains("type") and j.contains("value")) {
                switch (auto const type = j.at("type").get<std::string>(); cosmos::to_request_type(type)) {
                    case cosmos::request_type::dag:
                        request.data = j.get<cosmos::shyguy_dag>(); break;
                    case cosmos::request_type::task:
                        request.data = j.get<cosmos::shyguy_task>(); break;
                    case cosmos::request_type::unset:
                    case cosmos::request_type::unknown:
                    default: {
                        request.data = std::monostate();
                    }
                }
            }
        }
    };

}
