cmake_minimum_required(VERSION 3.9...3.29)


set(HEADER_DIR ${CMAKE_SOURCE_DIR}/libraries/cron_parser/)
FILE(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/libraries/cron_parser/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/libraries/cron_parser/*.hpp)

add_library(cron_parser STATIC ${SOURCE_FILES} ${HEADER_FILES})
set_target_properties(cron_parser PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(cron_parser PUBLIC ${HEADER_DIR})