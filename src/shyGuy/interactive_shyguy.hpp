//
// Created by Josue Herrera on 11/10/24.
//

#pragma once
#ifndef INTERACTIVE_SHYGUY_HPP
#define INTERACTIVE_SHYGUY_HPP


#include "blocking_queue.hpp"
#include "shyguy_request.hpp"
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Radiobox, Renderer, Tab, Toggle, Vertical
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for Element, separator, operator|, vbox, border
#include "ftxui/screen/color_info.hpp"
#include <ftxui/dom/table.hpp>      // for Table, TableSelection
#include <array>
#include <tui.h>
#include <string>
#include <memory>


namespace cosmos::inline v1 {

    template<class Enum>
    auto to_underlying_ptr(Enum& e) -> std::underlying_type_t<Enum>* {
        return reinterpret_cast<std::underlying_type_t<Enum>*>(e);
    }


class interactive_shyguy {
        using request_queue_t = std::shared_ptr<blocking_queue<std::pair<std::vector<task_runner>, directed_acyclic_graph>>>;
private:
    request_queue_t request_queue;
public:
    explicit interactive_shyguy(request_queue_t const& in): request_queue{in}{};
    inline auto runtui ()
    {
        tui app{};
        app.run();
    }
    inline auto demo () {
        using namespace ftxui;
        using namespace std::string_literals;

        // this must be on its own thread!
        auto screen = ScreenInteractive::Fullscreen();

        //The actions
        std::size_t action_index = 0;
        auto choice_type_entries = std::vector{" dag "s, " task "s};
        auto action_entries      = std::vector{" create "s," delete "s," run "s,};

        ftxui::Components action_components{};
        std::string create_name{};
        std::string create_schedule{};
        int choice_type_index{};

        auto name_input_component     = Input(&create_name, "name");
        auto schedule_input_component = Input(&create_schedule, "schedule (e.g. * * * * *)");
        auto choice_type_toggle       = Toggle(&choice_type_entries, &choice_type_index);
        auto const create_label       = "submit"s;

        auto create_button = Button(&create_label, [&] {
            request_queue->enqueue(
                {

                }
            );
        }, ButtonOption::Simple());

        auto create_component_tree = Container::Horizontal({
            Container::Vertical({
                name_input_component,
                schedule_input_component
            }),
            Container::Vertical({
                create_button
            })
        });

        action_components.emplace_back(
            Renderer(create_component_tree, [&] {
                return hbox({
                    vbox({
                        hbox(text("name: "), name_input_component->Render()),
                        hbox(text("schedule: "), schedule_input_component->Render())
                    }) | flex_grow,
                    separator(),
                    vbox({
                        create_button->Render()
                    }) | borderRounded
                });
            })
        );

        auto action_selection               = Menu(&action_entries, reinterpret_cast<int*>(&action_index), MenuOption::HorizontalAnimated());
        auto actions_tab_component          = Container::Tab(action_components, reinterpret_cast<int*>(&action_index));
        auto actions_tab_component_renderer = Renderer([&] {
            return action_components[action_index]->Render();
        });

        auto actions_component_tree = Container::Vertical({
            Container::Horizontal({choice_type_toggle, action_selection}),
            actions_tab_component
        });

        // Tweak how the component tree is rendered:
        auto actions_component_tree_renderer = Renderer(actions_component_tree, [&] {
              return vbox({
                    hbox({
                        choice_type_toggle->Render() | border,
                        action_selection->Render()
                    }),
                    separator(),
                    actions_tab_component_renderer->Render() | flex,
              });
        });

        // ---------------------------------------------------------------------------
        // Dag Selection menu
        // ---------------------------------------------------------------------------
        std::size_t dag_index = 0;

        // ---------------------------------------------------------------------------
        // task Selection menu
        // --------------------------------------------------------------------------
        auto task_index     = 0;
        auto task_entries   = std::vector {
            std::vector {"H1"s, "H2"s, "H3elllosadasasdasd"s},
            std::vector {"D1"s, "D2"s, "D3"s},
            std::vector {"W1"s, "W2"s, "W3"s},
            std::vector {"M1"s, "M2"s, "M3"s},
            std::vector {"W1"s, "W2"s, "W3"s},
            std::vector {"M1"s, "M2"s, "M3"s}
        };
        auto task_selection = std::vector {
            Menu(&task_entries[0], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[1], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[2], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[3], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[4], &task_index, MenuOption::Vertical()),
            Menu(&task_entries[5], &task_index, MenuOption::Vertical()),
        };

        auto task_tab_component          = Container::Tab(task_selection, &task_index);
        auto task_tab_component_renderer = Renderer(task_tab_component, [&]{
            return hbox({
                text(" name : " + task_entries[dag_index][static_cast<size_t>(task_index)])
            }) | flex_grow;
        });

        auto task_component_tree = Container::Horizontal({
            Container::Vertical({task_selection[dag_index]}),
            task_tab_component_renderer
        });

        // ---------------------------------------------------------------------------
        auto dag_entries   = std::vector{"hourly"s, "daily"s, "weekly"s, "monthly"s, "yearly"s, "custom"s};
        auto dag_selection = Menu(&dag_entries, reinterpret_cast<int*>(&dag_index), MenuOption::HorizontalAnimated());
        auto task_component_tree_renderer   = Renderer(task_component_tree, [&] {
            return hbox({
                vbox({
                    task_selection[dag_index]->Render()
                    | yframe
                    | yflex_grow
                    | xflex_shrink
                    | size(WIDTH, LESS_THAN, 11)
                }),
                separator(),
                vbox({
                    task_tab_component_renderer->Render() | flex
                })
            });
        });
        // ---------------------------------------------------------------------------

        auto dags_component_tree = Container::Vertical({
            Container::Horizontal({dag_selection}),
            task_component_tree_renderer
        });

        auto dags_component_tree_render = Renderer(dags_component_tree, [&]{
            return vbox({
                hbox({
                    dag_selection->Render() | flex
                }),
                hbox({
                    task_component_tree_renderer->Render()
                }) | flex_grow
            });
        });

        // ---------------------------------------------------------------------------
        // Tabs
        // ---------------------------------------------------------------------------
        int  dags_tab_index     = 0;
        auto dags_tab_entries   = std::vector{ "dags"s, "actions"s, "settings"s };
        auto dags_tab_selection = Menu(&dags_tab_entries, &dags_tab_index, MenuOption::HorizontalAnimated());
        auto dags_tab_component = Container::Tab({
            dags_component_tree_render,
            actions_component_tree_renderer
        }, &dags_tab_index);


        auto main_component_tree = Container::Vertical({
              Container::Horizontal({
                  dags_tab_selection
              }),
              dags_tab_component,
        });

        auto main_component_tree_renderer = Renderer(main_component_tree, [&] {
            return vbox({
                hbox({
                    dags_tab_selection->Render() | flex,
                }),
                dags_tab_component->Render() | flex,
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

        screen.Loop(main_component_tree_renderer);
        refresh_ui_continue = false;
        refresh_ui.join();
    }
};

} //namespace cosmos

#endif //INTERACTIVE_SHYGUY_HPP
