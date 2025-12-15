//
// Created by jaxma on 10/3/2025.
//

#include "horizontaltabpanel.h"

#include <algorithm>
#include <utility>
namespace cosmos::inline v1
{
    horizontal_tab_panel::horizontal_tab_panel(ftxui::Components components, std::vector<std::string> values)
        : names{std::move(values)}
    {
        pages = std::move(components);
        rebuild();
    }

    horizontal_tab_panel::horizontal_tab_panel(horizontal_tab_panel const& rhs)
    {
        *this = rhs;
    }

    horizontal_tab_panel::horizontal_tab_panel(horizontal_tab_panel&& rhs) noexcept
    {
        *this = std::move(rhs);
    }

    void horizontal_tab_panel::rebuild()
    {
        if (names.empty())
        {
            selected_tab = 0;
        }
        else
        {
            selected_tab = std::clamp(selected_tab, std::int32_t{0}, static_cast<std::int32_t>(names.size() - 1));
        }

        selection = ftxui::Menu(&names, &selected_tab);
        tab       = ftxui::Container::Tab(pages, &selected_tab);
        component = ftxui::Container::Horizontal({tab, selection});
        renderer  = ftxui::Renderer(component, [this]
        {
           return ftxui::hbox({
               selection->Render(),
               ftxui::separator(),
               tab->Render()
           });
        });
    }

    horizontal_tab_panel& horizontal_tab_panel::operator=(horizontal_tab_panel const & rhs)
    {
        if (this == &rhs)
            return *this;

        selected_tab = rhs.selected_tab;
        pages     = rhs.pages;
        names     = rhs.names;
        rebuild();
        return *this;

    }

    horizontal_tab_panel& horizontal_tab_panel::operator=(horizontal_tab_panel&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        selected_tab = rhs.selected_tab;
        pages        = std::move(rhs.pages);
        names        = std::move(rhs.names);
        rebuild();
        return *this;
    }
}
