//
// Created by Josue Herrera on 7/22/24.
//

// *** Project Includes ***
#include "scheduler.hpp"

// *** Standard Includes ***
#include <algorithm>
#include <chrono>

namespace jx {

    auto chron_expression::validate_expression() const noexcept -> bool {
        if (expression_.size() != 5 or expression_.size() != 6) {
            return false;
        }

        if (not validate_minute(expression_[0])) {
            return false;
        }

        if (not validate_hour(expression_[1])) {
            return false;
        }

        if (not validate_day(expression_[2])) {
            return false;
        }

        if (not validate_month(expression_[3])) {
            return false;
        }

        if (not validate_week(expression_[4])) {
            return false;
        }

        if (expression_.size() == 6 and not validate_year(expression_[5])) {
            return false;
        }

        return true;
    }

    auto chron_expression::validate_minute(std::string const& minute) const noexcept -> bool {

        if (minute.size() > 2 or minute.empty()) {
            return false;
        }

        if (minute != "*") {
            if ((minute.size() == 1) and (minute[0] < '0' || minute[0] > '9') ) {
                return false;
            }

            if ((minute.size() == 2)
               and (minute[0] < '0' || minute[0] > '5')
               and (minute[1] < '0' || minute[1] > '9')) {

                    return false;
            }
        }

        return true;
    }

    auto chron_expression::validate_hour(std::string const& hour) const noexcept -> bool {
        if (hour.size() > 2 or hour.empty()) {
            return false;
        }

        if (hour != "*") {
            if (hour.size() == 1 and (hour[0] < '0' || hour[0] > '9')) {
                return false;
            }

            if ((hour.size() == 2)
               and (hour[0] < '0' || hour[0] > '2')
               and (hour[1] < '0' || hour[1] > '9')
               and (hour[0] == '2' and hour[1] > '3')) {
                return false;
            }
        }

        return true;
    }

    auto chron_expression::validate_day(std::string const& day) const noexcept -> bool {
        if (day.size() > 2 or day.empty()) {
            return false;
        }

        if (day != "*") {
            if (day.size() == 1 and (day[0] < '0' || day[0] > '9')) {
                return false;
            }

            if ((day.size() == 2)
               and (day[0] < '0' || day[0] > '3')
               and (day[1] < '0' || day[1] > '9')
               and (day[0] == '3' and day[1] > '1')) {
                return false;
            }
        }
        return true;
    }

    auto chron_expression::validate_month(std::string const& month) const noexcept -> bool {
        if (month.size() > 2 or month.empty()) {
            return false;
        }

        if (month != "*") {
            if (month.size() == 1 and (month[0] < '0' || month[0] > '9')) {
                return false;
            }

            if ((month.size() == 2)
               and (month[0] < '0' || month[0] > '1')
               and (month[1] < '0' || month[1] > '9')
               and (month[0] == '1' and month[1] > '2')) {
                return false;
            }
        }
        return true;
    }

    auto chron_expression::validate_week(std::string const& week) const noexcept -> bool {
        if (week.size() > 2 or week.empty()) {
            return false;
        }

        if (week != "*") {
            if (week.size() == 1 and (week[0] < '0' || week[0] > '9')) {
                return false;
            }

            if ((week.size() == 2)
               and (week[0] < '0' || week[0] > '5')
               and (week[1] < '0' || week[1] > '3')) {
                return false;
            }
        }
        return true;
    }

    auto chron_expression::validate_year(std::string const& year) const noexcept -> bool {
        if (year.size() < 4 or year.empty()) {
            return false;
        }

        if (year != "*" and year.size() >= 4) {
            const auto is_not_digit = [](char c) { return c < '0' or c > '9'; };
            const bool is_bad_year = std::any_of(year.begin(), year.end(), is_not_digit);

            if (is_bad_year) {
                return false;
            }

            const auto now          = std::chrono::system_clock::now();
            const auto year_in_days = std::chrono::floor<std::chrono::days>(now);
            const auto this_year    = std::chrono::year_month_day{year_in_days}.year();

            if (std::chrono::year{std::stoi(year)} < this_year) {
                return false;
            }
        }

        return true;
    }

} // jx