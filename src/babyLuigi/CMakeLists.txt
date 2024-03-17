

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
FILE(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/babyLuigi/*.cc)
FILE(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.hpp)

add_executable(babyLuigi baby_main.cc ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(babyLuigi PRIVATE ${HEADER_DIR})

if(WIN32)
    target_compile_definitions(babyLuigi PUBLIC _WIN32_WINNT=0x0601)
    set(CMAKE_CXX_FLAGS "/await:strict /EHsc /wd4996")
else()

    set(CLANG_COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion;-Werror")
    set(CLANG_COMPILE_DEBUG_OPTIONS "${CLANG_COMPILE_OPTIONS};-O0;")
    set(CLANG_COMPILE_RELEASE_OPTIONS "${CLANG_COMPILE_OPTIONS};-O3;")

    target_compile_options(babyLuigi PUBLIC "$<$<CONFIG:Debug>:${CLANG_COMPILE_DEBUG_OPTIONS}>")
    target_compile_options(babyLuigi PUBLIC "$<$<CONFIG:Release>:${CLANG_COMPILE_DEBUG_OPTIONS}>")
endif()

target_link_system_libraries(
    babyLuigi
  PRIVATE
    CLI11::CLI11
    fmt::fmt
    spdlog::spdlog
    range-v3::range-v3
    json::json
    libzmq-static
    cppzmq-static
)
