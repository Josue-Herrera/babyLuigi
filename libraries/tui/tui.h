//
// Created by jojo on 10/3/2025.
//
#pragma once
#ifndef TUI_H
#define TUI_H

// *** Project ***
#include <fwd_vocabulary.hpp>

// *** Standard ***
#include <atomic>
#include <thread>

namespace cosmos::inline v1
{

class tui
{
public:
    void run(concurrent_shyguy_t shyguy, input_queue_t io_queue);
    std::int32_t shift {};
    std::atomic_bool refresh{true};
    std::thread refresh_thread;
};

}

#endif //TUI_H
