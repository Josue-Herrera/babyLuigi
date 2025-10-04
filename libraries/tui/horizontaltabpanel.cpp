//
// Created by jaxma on 10/3/2025.
//

#include "horizontaltabpanel.h"

#include <utility>
namespace cosmos::inline v1
{

    horizontal_tab_panel::horizontal_tab_panel(ftxui::Components components, std::vector<std::string> values)
        :   names{std::move(values)}
    {
        selection = ftxui::Menu(&names, & selected_tab);
        tab       = ftxui::Container::Tab(std::move(components), &selected_tab);
        component = ftxui::Container::Horizontal({tab, selection});
        renderer = ftxui::Renderer(component, [&]
        {
           return ftxui::hbox({
               selection->Render(),
               ftxui::separator(),
               tab->Render()
           });
        });
    }

}