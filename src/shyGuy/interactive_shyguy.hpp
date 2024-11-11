//
// Created by Josue Herrera on 11/10/24.
//

#pragma once
#ifndef INTERACTIVE_SHYGUY_HPP
#define INTERACTIVE_SHYGUY_HPP

#include <ftxui/dom/elements.hpp>  // for text, center, separator, operator|, flex, Element, vbox, Fit, hbox, border
#include <ftxui/screen/screen.hpp>  // for Full, Screen
#include <memory>                   // for allocator

#include <ftxui/dom/node.hpp>      // for Render
#include <ftxui/screen/color.hpp>  // for ftxui


namespace cosmos::inline v1 {

class interactive_shyguy {

    inline demo () {
        using namespace ftxui;
        auto document = hbox({
                            text("left-column"),
                            separator(),
                            vbox({
                                center(text("top")) | flex,
                                separator(),
                                center(text("bottom")),
                            }) | flex,
                            separator(),
                            text("right-column"),
                        }) |
                        border;
        auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
        Render(screen, document);
        screen.Print();
    }
};

} //namespace cosmos

#endif //INTERACTIVE_SHYGUY_HPP
