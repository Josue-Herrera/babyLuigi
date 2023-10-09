
#include "task_system/task_maintainer.hpp"
#include <nlohmann/json.hpp>

namespace jx {
inline namespace v1 {

using json = nlohmann::json;

auto task_maintainer::service(task_view view) noexcept -> bool {
  json value = json::parse(view.request);

  return value.contains("dependencies");
}

} // namespace v1
} // namespace jx
