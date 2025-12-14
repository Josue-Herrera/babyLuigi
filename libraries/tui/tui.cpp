//
// Created by jojo on 10/3/2025.
//
#include "tui.h"

// *** Project ***
#include "homepage.h"
#include "dagpanel.h"
#include "horizontaltabpanel.h"

// *** 3rdparty ***
#include <ftxui/component/screen_interactive.hpp>

// *** Standard ***
#include <chrono>

namespace cosmos::inline v1
{

static const std::vector<std::string> actions_entries {
    "create", "delete", "update", "execute", "restart","snapshot"
};

void tui::run(concurrent_shyguy_t shyguy)
{
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    dag_panel dag_pages_panel{shyguy};
    horizontal_tab_panel actions {{}, actions_entries};

    homepage home{{
        dag_pages_panel.render(),
        actions.render()
    }};

    refresh_thread = std::thread([&]{
        while (refresh)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            screen.Post([&]{shift++;});
            // here we can update the dag_panel actually.
            screen.Post(ftxui::Event::Custom);
        }
    });
;
    screen.Loop(home.render());
    refresh = false;

}

}
