//
// Created by jojo on 10/4/2025.
//
#pragma once
#ifndef DAGPAGE_H
#define DAGPAGE_H

// *** Project ***
#include "horizontaltabpanel.h"
#include <fwd_vocabulary.hpp>
#include <storage.hpp>

// *** 3rdparty ****
#include <ftxui/component/component.hpp>

// *** Standard ****
#include <ranges>

namespace cosmos::inline v1
{

class dag_page
{
public:
    dag_page(schedule_frequency schedule, concurrent_shyguy_t const& shy);
    schedule_frequency frequency{schedule_frequency::custom};
    ftxui::Component content{};
    ftxui::Component renderer{};
    std::int32_t selected_dag{};
    std::vector<std::string> names{};
    std::vector<std::vector<std::string>> data{};
private:
    [[nodiscard]] auto render(concurrent_shyguy_t shyguy) -> ftxui::Element;
};

inline auto dag_pages(
    concurrent_shyguy_t const& shyguy,
    std::vector<schedule_frequency> const& data) -> std::vector<dag_page>
{
    return std::views::transform(data, [&shyguy](auto const& item)
    {
        return dag_page{item, shyguy};
    })
    | std::ranges::to<std::vector>();
}

}

#endif //DAGPAGE_H
