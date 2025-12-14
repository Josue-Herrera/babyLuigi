//
// Created by jojo on 10/3/2025.
//
#pragma once
#ifndef HOMEPAGE_H
#define HOMEPAGE_H

// *** 3rdparty ****
#include <ftxui/component/component.hpp>

// *** Standard ***
#include <string>
#include <vector>

namespace cosmos::inline v1
{

class homepage
{
    inline static const std::vector<std::string> homepage_tabs {" dags ", " actions "};
public:

    explicit homepage(ftxui::Components);
    [[nodiscard]] auto render() -> ftxui::Component { return homepage_render; };
private:
    std::int32_t  select_homepage_id {};
    ftxui::Component homepage_selection{};
    ftxui::Component homepage_tab{};
    ftxui::Component homepage_component{};
    ftxui::Component homepage_render{};
};


} // namespace cosmos::v1

#endif //HOMEPAGE_H
