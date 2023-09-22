cmake_minimum_required(VERSION 3.22...3.27)

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
  babyLuigi_shyGuy
  VERSION 0.0.1
  DESCRIPTION "simple task scheduling system"
  LANGUAGES CXX)

include(cmake/Dependencies.cmake)
setup_dependencies()

include(cmake/SystemLink.cmake)
add_subdirectory(src)

# Don't even look at tests if we're not top level
# if(NOT PROJECT_IS_TOP_LEVEL)
#   return()
# endif()

# # Adding the tests:
# include(CTest)

# if(BUILD_TESTING)
#   add_subdirectory(test)
# endif()

