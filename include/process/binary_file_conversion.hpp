#pragma once 

// 3rdparty Inc
#include <range/v3/view/drop_while.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/common.hpp>

// Standard Inc
#include <vector>
#include <string>
#include <string_view>

namespace jx {
    inline namespace v1 {

        
        [[nodiscard]] inline constexpr auto binary_file_conversion(std::string const& input) noexcept -> std::string 
        {
            using namespace ranges;

            auto to_characters = [](auto splitted_view)
            {
                auto bytes = splitted_view | views::common | to<std::string>();
                return static_cast<char>(std::stoi(bytes));
            };

            auto file = input
                | views::drop_while([](char c) { return c == '[' || c == ']'; })
                | views::split(',')
                | views::transform(to_characters)
                | to<std::string>();

            return file;
        }
    }
}

