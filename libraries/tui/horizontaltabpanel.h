//
// Created by jojo on 10/3/2025.
//
#pragma once
#ifndef HORIZONTALTABPANEL_H
#define HORIZONTALTABPANEL_H

// *** 3rdparty ****
#include <ftxui/component/component.hpp>

namespace cosmos::inline v1
{

class horizontal_tab_panel
{
public:
    horizontal_tab_panel(ftxui::Components components, std::vector<std::string> values);
    [[nodiscard]] auto render() -> ftxui::Component { return renderer; };
private:
    std::int32_t selected_tab{};
    ftxui::Component selection;
    ftxui::Component tab;
    ftxui::Component component;
    ftxui::Component renderer{};
    const std::vector<std::string> names;
};

}

#endif //HORIZONTALTABPANEL_H
