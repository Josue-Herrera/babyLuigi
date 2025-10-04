//
// Created by jojo on 10/3/2025.
//
#include "tui.h"

// *** Project ***
#include "homepage.h"
#include "horizontaltabpanel.h"

// *** 3rdparty ***
#include <ftxui/component/screen_interactive.hpp>

// *** Standard ***
#include <chrono>

namespace cosmos::inline v1
{

static const std::vector<std::string> dag_entries {
    "hourly", "daily", "weekly", "monthly", "yearly", "custom"
};

static const std::vector<std::string> actions_entries {
    "create", "delete", "restart","snapshot", "update"
};

void tui::run()
{
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    horizontal_tab_panel dags {{}, dag_entries};
    horizontal_tab_panel actions {{}, actions_entries};

    homepage home{{
        dags.render(),
        actions.render()
    }};

    refresh_thread = std::thread([&]{
        while (refresh)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            screen.Post([&]{shift++;});
            screen.Post(ftxui::Event::Custom);
        }
    });
;
    screen.Loop(home.render());
    refresh = false;

}

}