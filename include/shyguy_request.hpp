#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <nlohmann/json.hpp>

namespace cosmos::inline v1 {

    template <typename... Fs>
    struct match : Fs... {
        using Fs::operator()...;
        constexpr match(Fs &&... fs) : Fs{fs}... {}
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

    inline std::string to_string(request_type const type) {
        using namespace std::string_literals;
        switch (type) {
            case request_type::dag      : return "dag"s;
            case request_type::task       : return "task"s;
            case request_type::unset       : return "unset"s;
            case request_type::unknown     : return "unknown"s;
            default : return "unknown"s;
        }
    }

    inline request_type to_request_type(std::string const& str) {
        using namespace std::string_literals;
        if (str ==  "dag"s) return request_type::dag;
        if (str ==  "task"s) return request_type::task;
        if (str ==  "unset"s)  return request_type::unset;
        return request_type::unknown;
    }

    inline std::string to_string(task_type const type) {
        using namespace std::string_literals;
        switch (type) {
            case task_type::python      : return "python"s;
            case task_type::shell       : return "shell"s;
            case task_type::unset       : return "unset"s;
            case task_type::unknown     : return "unknown"s;
            default : return "unknown"s;
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
        static void from_json(const json& j, const std::monostate&) {}
    };

    template <>
    struct adl_serializer<cosmos::shyguy_request> {
        static void to_json(json& j, cosmos::shyguy_request const& request) {
            auto to_json_internal = [&]<typename T0>(T0&& value) {
                const auto request_enum = static_cast<cosmos::request_type>(request.data.index());
                j["type"] = cosmos::to_string(request_enum);
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
