//
// Created by Josue Herrera on 11/10/24.
//

#pragma once
#ifndef INTERACTIVE_SHYGUY_HPP
#define INTERACTIVE_SHYGUY_HPP

#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Renderer, Tab, Toggle, Vertical
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for Element, separator, operator|, vbox, border
#include "ftxui/screen/color_info.hpp"
#include <ftxui/dom/table.hpp>      // for Table, TableSelection

namespace cosmos::inline v1 {

class interactive_shyguy {

    // different types of pages as functions
    //  main page ->
public:
    inline auto demo () {
        using namespace ftxui;
        using namespace std::string_literals;

        // this must be on its own thread!
        auto screen = ScreenInteractive::Fullscreen();

        // The data:
        std::string first_name;
        std::string last_name;
        std::string password{"hello?"};
        std::string phoneNumber{"9098005550"};

        // The basic input components:
        Component input_first_name = Input(&first_name, "first name");
        Component input_last_name = Input(&last_name, "last name");

        // The component tree:
        auto component = Container::Vertical({
            input_first_name,
            input_last_name
        });

        // Tweak how the component tree is rendered:
        auto dags_view_renderer = Renderer(component, [&] {
          return vbox({
                     hbox(text(" First name : "), input_first_name->Render()),
                     hbox(text(" Last name  : "), input_last_name->Render()),
                     separator(),
                     text("Hello " + first_name + " " + last_name),
                     text("Your password is " + password),
                     text("Your phone number is " + phoneNumber),
                 });
        });



        // ---------------------------------------------------------------------------
        // Dag Selection menu
        // ---------------------------------------------------------------------------
        int  dag_index     = 0;

        // ---------------------------------------------------------------------------
        // task Selection menu
        // ---------------------------------------------------------------------------
        int  task_index     = 0;
        auto task_entries   = std::vector {
            std::vector {"H1"s, "H2"s, "H3elllosadasasdasd"s},
            std::vector {"D1"s, "D2"s, "D3"s},
            std::vector {"W1"s, "W2"s, "W3"s},
            std::vector {"M1"s, "M2"s, "M3"s}
        };
        auto task_selection = std::vector {
            Menu(&task_entries[0], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[1], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[2], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[3], &task_index, MenuOption::Vertical()),
        };

        auto task_tab = Container::Tab(task_selection, &task_index);
        auto task_tab_content = Renderer(task_tab, [&]{
            return hbox({
                text(" name : " +  task_entries[static_cast<size_t>(dag_index)][static_cast<size_t>(task_index)])
            }) | flex_grow;
        });

        auto task_container = Container::Horizontal({
            Container::Vertical({task_selection[static_cast<size_t>(dag_index)]}),
            task_tab_content
        });
        // ---------------------------------------------------------------------------

        auto dag_entries   = std::vector{"hourly"s, "daily"s, "weekly"s, "monthly"s};
        auto dag_selection = Menu(&dag_entries, &dag_index, MenuOption::HorizontalAnimated());
        auto dag_content   = Renderer(task_container, [&] {
            return hbox({
                vbox({
                    task_selection[static_cast<size_t>(dag_index)]->Render()
                }) | yflex_grow | xflex_shrink | size(WIDTH, LESS_THAN, 11),
                separator(),
                vbox({
                    task_tab_content->Render()
                })
            });
        });
        // ---------------------------------------------------------------------------


        // ---------------------------------------------------------------------------
        // Composing Content Together (dag_selection, dag_content)
        // ---------------------------------------------------------------------------
        auto dag_container = Container::Vertical({
            Container::Horizontal({dag_selection}),
            dag_content
        });

        auto dag_view_rr = Renderer(dag_container, [&]{
            return vbox({
                hbox({
                    dag_selection->Render() | flex
                }),
                hbox({
                    dag_content->Render()
                }) | flex
            }) | xflex_shrink;
        });

    // ---------------------------------------------------------------------------
    // Tabs
    // ---------------------------------------------------------------------------
    int  tab_index     = 0;
    auto tab_entries   = std::vector{ "DAGs"s, "Actions"s, "Settings"s };
    auto tab_selection = Menu(&tab_entries, &tab_index, MenuOption::HorizontalAnimated());
    auto tab_content   = Container::Tab(
    {
        dag_view_rr, dags_view_renderer
    },
    &tab_index);


  auto main_container = Container::Vertical({
      Container::Horizontal({
          tab_selection
      }),
      tab_content,
  });

  auto main_renderer = Renderer(main_container, [&] {
    return vbox({
        hbox({
            tab_selection->Render() | flex,
        }),
        tab_content->Render() | flex,
    })| borderRounded;
  });

  int shift = 0;
  std::atomic<bool> refresh_ui_continue = true;
  std::thread refresh_ui([&] {
    while (refresh_ui_continue) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(0.05s);
      // The |shift| variable belong to the main thread. `screen.Post(task)`
      // will execute the update on the thread where |screen| lives (e.g. the
      // main thread). Using `screen.Post(task)` is threadsafe.
      screen.Post([&] { shift++; });
      // After updating the state, request a new frame to be drawn. This is done
      // by simulating a new "custom" event to be handled.
      screen.Post(Event::Custom);
    }
  });

        screen.Loop(main_renderer);
        refresh_ui_continue = false;
        refresh_ui.join();
    }
};

} //namespace cosmos

#endif //INTERACTIVE_SHYGUY_HPP
