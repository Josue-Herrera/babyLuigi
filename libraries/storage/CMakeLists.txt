cmake_minimum_required(VERSION 3.22...3.29)

set(HEADER_DIR ${CMAKE_SOURCE_DIR}/libraries/storage)
file(GLOB STORAGE_HEADERS ${HEADER_DIR}/*.hpp)
file(GLOB STORAGE_SOURCES ${HEADER_DIR}/*.cpp)

add_library(storage STATIC ${STORAGE_HEADERS} ${STORAGE_SOURCES})
set_target_properties(storage PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(storage PUBLIC ${HEADER_DIR} ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(storage PUBLIC json::json)

