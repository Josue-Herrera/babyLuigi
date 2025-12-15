//
// Created by jojo on 10/3/2025.
//
#pragma once
#ifndef HORIZONTALTABPANEL_H
#define HORIZONTALTABPANEL_H

// *** 3rdparty ****
#include <ftxui/component/component.hpp>

#include "fwd_vocabulary.hpp"

namespace cosmos::inline v1
{

class horizontal_tab_panel
{

public:
    horizontal_tab_panel() = default;
    horizontal_tab_panel(ftxui::Components components, std::vector<std::string> values);
    horizontal_tab_panel(horizontal_tab_panel const&);
    horizontal_tab_panel(horizontal_tab_panel&&) noexcept;

    horizontal_tab_panel& operator=(horizontal_tab_panel const&);
    horizontal_tab_panel& operator=(horizontal_tab_panel&&) noexcept;

    [[nodiscard]] auto render() const -> ftxui::Component { return renderer; };
    [[nodiscard]] auto selected() const -> const std::int32_t * {return &selected_tab;};
private:
    void rebuild();

    std::int32_t selected_tab{};
    ftxui::Components pages;
    ftxui::Component selection;
    ftxui::Component tab;
    ftxui::Component component;
    ftxui::Component renderer{};
    std::vector<std::string> names;
};

}

#endif //HORIZONTALTABPANEL_H
