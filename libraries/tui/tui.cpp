//
// Created by jojo on 10/3/2025.
//
#include "tui.h"

// *** Project ***
#include "homepage.h"
#include "dagpanel.h"
#include "actionspanel.h"

// *** 3rdparty ***
#include <ftxui/component/screen_interactive.hpp>

// *** Standard ***
#include <chrono>

namespace cosmos::inline v1
{

void tui::run(concurrent_shyguy_t shyguy, input_queue_t io_queue)
{
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    dag_panel dag_pages_panel{shyguy};

    actions_panel actions{shyguy, io_queue};

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

    if (refresh_thread.joinable())
        refresh_thread.join();

}

}
