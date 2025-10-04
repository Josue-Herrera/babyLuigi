//
// Created by jojo on 10/3/2025.
//

#include "homepage.h"

// *** 3rdparty ****
#include <ftxui/component/component.hpp>


#include <utility>

namespace cosmos::inline v1
{
    /**
     * 
     * @param components 
     */
    homepage::homepage(ftxui::Components components)
    {
        auto menu_options = ftxui::MenuOption::HorizontalAnimated();
        menu_options.underline.color_active = ftxui::Color::BlueViolet;

        homepage_selection = ftxui::Menu(&homepage_tabs, &select_homepage_id, menu_options);
        homepage_tab       = ftxui::Container::Tab(std::move(components), &select_homepage_id);
        homepage_component = ftxui::Container::Vertical({
            ftxui::Container::Horizontal({homepage_selection}),
            homepage_tab
        });

        homepage_render = ftxui::Renderer(homepage_component, [&]
        {
           return ftxui::vbox({
                ftxui::hbox({homepage_selection->Render() | ftxui::flex}),
               homepage_tab->Render() | ftxui::flex
           });
        });
    }

}