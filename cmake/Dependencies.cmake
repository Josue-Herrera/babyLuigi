include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project
  
  if(NOT TARGET fmtlib::fmtlib)
    cpmaddpackage("gh:fmtlib/fmt#9.1.0")
  endif()

  if(NOT TARGET spdlog::spdlog)
    cpmaddpackage(
      NAME
      spdlog
      VERSION
      1.11.0
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

  if(NOT TARGET asio::asio)
    find_package(Threads REQUIRED)
    cpmaddpackage("gh:chriskohlhoff/asio#asio-1-28-0@1.28.0")
    if(asio_ADDED)
      add_library(asio INTERFACE)
      add_library(asio::asio ALIAS asio)
      target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
      target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
      target_link_libraries(asio INTERFACE Threads::Threads)
    endif()
  endif()

endfunction()
