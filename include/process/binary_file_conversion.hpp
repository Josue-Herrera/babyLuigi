#pragma once

// 3rdparty Inc
#include <nlohmann/json.hpp>
#include <range/v3/view/drop_while.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/common.hpp>

// Standard Inc
#include <vector>
#include <string>
#include <string_view>

namespace cosmos {
    inline namespace v1 {


        [[nodiscard]] inline auto binary_file_conversion(nlohmann::json const& input, std::string const&where ) noexcept -> std::string
        {
            auto file = input[where]
                | ranges::views::transform([](auto const& i) { return  i.template get<char>(); })
                | ranges::to<std::string>();

            return file;
        }
    }
}
