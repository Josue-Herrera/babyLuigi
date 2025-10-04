
set(HEADER_DIR ${CMAKE_SOURCE_DIR}/include)


FILE(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/shyGuy/*.cc)
FILE(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.hpp ${CMAKE_SOURCE_DIR}/src/shyGuy/*.hpp)

add_executable(shyguy shy_main.cc ${SOURCE_FILES} ${HEADER_FILES}
        shyexecutioner.cc
        shyexecutioner.hpp)

target_include_directories(shyguy PRIVATE ${HEADER_DIR})

if (WIN32)
    #target_compile_definitions(shyGuy PUBLIC _WIN32_WINNT=0x0601)
else()

    set(CLANG_COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Werror")
    set(CLANG_COMPILE_DEBUG_OPTIONS "${CLANG_COMPILE_OPTIONS};-O0;")
    set(CLANG_COMPILE_RELEASE_OPTIONS "${CLANG_COMPILE_OPTIONS};-O3;")

    target_compile_options(shyguy PUBLIC "$<$<CONFIG:Debug>:${CLANG_COMPILE_DEBUG_OPTIONS}>")
    target_compile_options(shyguy PUBLIC "$<$<CONFIG:Release>:${CLANG_COMPILE_DEBUG_OPTIONS}>")
endif ()

target_link_libraries(
        shyguy
        PRIVATE
        cron_parser
        storage
        tui
)

target_link_system_libraries(
        shyguy
        PRIVATE
        CLI11::CLI11
        fmt::fmt
        spdlog::spdlog
        range-v3::range-v3
        json::json
        libzmq-static
        cppzmq-static
        STDEXEC::stdexec
        ftxui::screen
        ftxui::dom
        ftxui::component
        uuid::uuid
)

