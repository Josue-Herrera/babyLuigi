cmake_minimum_required(VERSION 3.22...3.29)

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/libraries/threadsafe_shyguy)
file(GLOB STORAGE_HEADERS ${HEADER_DIR}/*.hpp)
file(GLOB STORAGE_SOURCES ${HEADER_DIR}/*.cpp)

add_library(threadsafe_shyguy STATIC ${STORAGE_HEADERS} ${STORAGE_SOURCES})
set_target_properties(threadsafe_shyguy PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(threadsafe_shyguy PUBLIC
        ${HEADER_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libraries
)

target_link_libraries(
        threadsafe_shyguy
        PRIVATE
        storage
)

target_link_system_libraries(
        threadsafe_shyguy
        PUBLIC
        spdlog::spdlog
        CLI11::CLI11
        range-v3::range-v3
)
