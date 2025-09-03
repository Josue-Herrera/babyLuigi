cmake_minimum_required(VERSION 3.22...3.29)

# Ensure dependencies are available (Catch2 added in setup_dependencies when BUILD_TESTING)

# Make Catch2 CTest integration available
if(TARGET Catch2::Catch2WithMain)
  # Catch2 provides extra CMake helpers in extras/Catch.cmake
  # Use module path approach for cleanliness
  get_target_property(_catch2_source_dir Catch2::Catch2 SOURCE_DIR)
  if(NOT _catch2_source_dir)
    # Fallback for some CPM setups
    set(_catch2_source_dir ${Catch2_SOURCE_DIR})
  endif()
  if(EXISTS "${_catch2_source_dir}/extras/Catch.cmake")
    list(APPEND CMAKE_MODULE_PATH "${_catch2_source_dir}/extras")
    include(Catch)
  endif()
endif()

add_executable(unit_tests
  test_basic.cpp
  test_zmq_babyluigi.cpp
  test_zmq_router.cpp
  ${CMAKE_SOURCE_DIR}/src/babyLuigi/baby_luigi.cc
  ${CMAKE_SOURCE_DIR}/src/shyGuy/zmq_router.cc
)

target_include_directories(unit_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(unit_tests PRIVATE
  Catch2::Catch2WithMain
  CLI11::CLI11
  STDEXEC::stdexec
  fmt::fmt
  spdlog::spdlog
  json::json
  libzmq-static
  cppzmq-static
)

# Automatically register tests with CTest if helpers are available
if(COMMAND catch_discover_tests)
  catch_discover_tests(unit_tests)
else()
  add_test(NAME unit_tests COMMAND unit_tests)
endif()
