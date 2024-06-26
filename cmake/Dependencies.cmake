include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project
  
  if(NOT TARGET fmtlib::fmtlib)
    cpmaddpackage("gh:fmtlib/fmt#10.2.1")
  endif()

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.13.0
      GITHUB_REPOSITORY
      "gabime/spdlog"
      OPTIONS
      "SPDLOG_FMT_EXTERNAL ON")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.3.2") 
  endif()

  if(NOT TARGET CLI11::CLI11)
    cpmaddpackage("gh:CLIUtils/CLI11@2.3.2")
  endif()

  if(NOT TARGET range-v3::range-v3)
    cpmaddpackage("gh:ericniebler/range-v3#0.12.0")
  endif()

  if(NOT TARGET json::json)
    cpmaddpackage("gh:nlohmann/json@3.10.5")
    add_library(json::json ALIAS nlohmann_json)
  endif()

  if(NOT TARGET zmq::zmq)
    # This is to add cmake/FindZeroMQ.cmake
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
    set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "doc" FORCE)
    set(CATCH_BUILD_EXAMPLES OFF CACHE BOOL "doc" FORCE)
    find_package(ZeroMQ REQUIRED)
    cpmaddpackage(
      NAME
      cppzmq
      VERSION
      4.10.0
      GITHUB_REPOSITORY
      "zeromq/cppzmq"
      OPTIONS
      "-DCPPZMQ_BUILD_TESTS=OFF" "-DCATCH_BUILD_EXAMPLES=OFF" 
    )
    unset(CPPZMQ_BUILD_TESTS CACHE)
    unset(CATCH_BUILD_EXAMPLES CACHE)
  endif()

  if(NOT TARGET tl::expected)
    cpmaddpackage("gh:TartanLlama/expected@1.1.0")
  endif()
    

endfunction()
