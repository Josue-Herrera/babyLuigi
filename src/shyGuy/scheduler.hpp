//
// Created by Josue Herrera on 7/22/24.
//
#pragma once

// *** Standard Includes ***
#include <string>
#include <vector>

namespace jx {
    inline namespace v1 {

        class schedule {

        };

        class chron_expression {
        public:
            explicit chron_expression(std::vector<std::string> const& expression)
                : expression_{expression} {}

            ~chron_expression() = default;
            chron_expression(chron_expression const&) = delete;
            chron_expression(chron_expression&&) = delete;
            chron_expression& operator=(chron_expression const&) = delete;
            chron_expression& operator=(chron_expression&&) = delete;

            [[nodiscard]] auto validate_minute(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_hour(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_day(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_month(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_week(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_year(std::string const&) const noexcept -> bool;
            [[nodiscard]] auto validate_expression() const noexcept -> bool;

        private:
            std::vector<std::string> expression_;
        };


        class scheduler {


            enum class occurrence { monthly, weekly, daily, hourly };


        };

    } // namespace v1
} // jx

