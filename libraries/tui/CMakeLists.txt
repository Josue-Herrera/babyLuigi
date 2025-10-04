cmake_minimum_required(VERSION 3.22...3.29)

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/libraries/tui)
file(GLOB STORAGE_HEADERS ${HEADER_DIR}/*.hpp)
file(GLOB STORAGE_SOURCES ${HEADER_DIR}/*.cpp)

add_library(tui STATIC ${STORAGE_HEADERS} ${STORAGE_SOURCES})
set_target_properties(tui PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(tui PUBLIC ${HEADER_DIR} ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(
        tui
        PRIVATE
        cron_parser
)

target_link_system_libraries(
        tui
        PUBLIC
        ftxui::screen
        ftxui::dom
        ftxui::component
)

