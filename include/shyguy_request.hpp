#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>


namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
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
                opt.emplace(j.template get<T>()); // same as above, but with
                                           // adl_serializer<T>::from_json
            }
        }
    };
}



namespace cosmos::inline v1
{
    enum class request_type { unset, dag, task, unknown };
    enum class task_type { unset, python, shell, placeholder, unknown };

    inline std::string to_string(task_type const type) {
        using namespace std::string_literals;

        switch (type) {
            case task_type::python      : return "python"s;
            case task_type::shell       : return "shell"s;
            case task_type::placeholder : return "placeholder"s;
            case task_type::unset       : return "unset"s;
            case task_type::unknown     : return "unknown"s;
            default : return "unknown"s;
        }
    }

    inline task_type to_enum(std::string const& str) {
            using namespace std::string_literals;
            if (str ==  "python"s) return task_type::python;
            if (str ==  "shell"s) return task_type::shell;
            if (str ==  "placeholder"s)  return task_type::placeholder;
            if (str ==  "unset"s)  return task_type::unset;
            return task_type::unknown;
    }

    class shyguy_request
    {
    public:
        //uuid
        request_type kind{};
        std::string name{};
        std::string type{};

        std::optional<std::string> schedule{};
        std::optional<std::string> filename{};
        std::optional<std::string> file_content{};
        std::optional<std::vector<std::string>> dependency_names{};
    };

    inline auto is_dag(shyguy_request const& r) -> bool {
        return r.kind == request_type::dag;
    }

    inline auto is_task(shyguy_request const& r) -> bool {
        return r.kind == request_type::task;
    }

    inline auto has_schedule(shyguy_request const& r) -> bool {
        return r.schedule.has_value() and not r.schedule.value().empty();
    }

    inline auto has_dependencies(shyguy_request const& t) -> bool {
        return t.dependency_names.has_value() and not t.dependency_names.value().empty();
    }

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(shyguy_request, kind, name, type, schedule, filename, file_content, dependency_names)
} // namespace cosmos::inline v1

