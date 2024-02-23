
#include "task_system/task_maintainer.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace jx {
inline namespace v1 {

using json = nlohmann::json;

[[nodiscard]] auto valid(json const & json_task) noexcept -> bool {
  return json_task.contains("dependencies") && 
        json_task.contains("name") && 
        json_task.contains("type") &&
        json_task.contains("payload");
}


[[nodiscard]] auto task_maintainer::service(task_view view) noexcept -> bool {
  json json_task = json::parse(view.request);

  if(not valid(json_task)) {
    spdlog::warn("json task {} is invalid");
    return false;
  }

  return true;
}

} // namespace v1
} // namespace jx
