cmake_minimum_required(VERSION 3.22...3.29)

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/libraries/tui)

set(STORAGE_HEADERS
        ${HEADER_DIR}/actionspanel.h
        ${HEADER_DIR}/dagpage.h
        ${HEADER_DIR}/dagpanel.h
        ${HEADER_DIR}/homepage.h
        ${HEADER_DIR}/horizontaltabpanel.h
        ${HEADER_DIR}/tui.h
)

set(STORAGE_SOURCES
        ${HEADER_DIR}/actionspanel.cpp
        ${HEADER_DIR}/dagpage.cpp
        ${HEADER_DIR}/dagpanel.cpp
        ${HEADER_DIR}/homepage.cpp
        ${HEADER_DIR}/horizontaltabpanel.cpp
        ${HEADER_DIR}/tui.cpp
)

add_library(tui STATIC ${STORAGE_HEADERS} ${STORAGE_SOURCES})
set_target_properties(tui PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(tui PUBLIC
        ${HEADER_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libraries/
)

target_link_libraries(
        tui
        PRIVATE
        cron_parser
        storage
        threadsafe_shyguy
)

target_link_system_libraries(
        tui
        PUBLIC
        ftxui::screen
        ftxui::dom
        ftxui::component
        spdlog::spdlog
        CLI11::CLI11
)

