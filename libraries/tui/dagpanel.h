//
// Created by jojo on 10/4/2025.
//
#pragma once
#ifndef DAGPANEL_H
#define DAGPANEL_H

// *** Project ***
#include "dagpage.h"
#include "horizontaltabpanel.h"

// *** 3rdparty ****
#include <ftxui/component/component.hpp>

#include "fwd_vocabulary.hpp"

namespace cosmos::inline v1
{

class dag_panel
{

    public:
    [[nodiscard]] auto render() -> ftxui::Component { return menu_tab.render(); };
    void update();
    explicit dag_panel(concurrent_shyguy_t const& shy);

private:
    std::vector<horizontal_tab_panel> pages{};
    horizontal_tab_panel menu_tab{};
    concurrent_shyguy_t shyguy;
};

}

#endif //DAGPANEL_H
