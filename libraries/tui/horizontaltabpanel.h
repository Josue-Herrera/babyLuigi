//
// Created by jojo on 10/3/2025.
//
#pragma once
#ifndef HORIZONTALTABPANEL_H
#define HORIZONTALTABPANEL_H

// *** 3rdparty ****
#include <ranges>
#include <ftxui/component/component.hpp>

#include "fwd_vocabulary.hpp"

namespace cosmos::inline v1
{

class horizontal_tab_panel
{

public:
    horizontal_tab_panel() = default;
    horizontal_tab_panel(ftxui::Components components, std::vector<std::string> values);
    horizontal_tab_panel& operator=(horizontal_tab_panel const&);
    [[nodiscard]] auto render() -> ftxui::Component { return renderer; };
    [[nodiscard]] auto selected() const -> const std::int32_t * {return &selected_tab;};
private:
    std::int32_t selected_tab{};
    ftxui::Component selection;
    ftxui::Component tab;
    ftxui::Component component;
    ftxui::Component renderer{};
    std::vector<std::string> names;
};

}

#endif //HORIZONTALTABPANEL_H
