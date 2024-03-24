#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libzmq" for configuration "Debug"
set_property(TARGET libzmq APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/libzmq-v143-mt-gd-4_3_6.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/libzmq-v143-mt-gd-4_3_6.dll"
  )

list(APPEND _cmake_import_check_targets libzmq )
list(APPEND _cmake_import_check_files_for_libzmq "${_IMPORT_PREFIX}/lib/libzmq-v143-mt-gd-4_3_6.lib" "${_IMPORT_PREFIX}/bin/libzmq-v143-mt-gd-4_3_6.dll" )

# Import target "libzmq-static" for configuration "Debug"
set_property(TARGET libzmq-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX;RC"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libzmq-v143-mt-sgd-4_3_6.lib"
  )

list(APPEND _cmake_import_check_targets libzmq-static )
list(APPEND _cmake_import_check_files_for_libzmq-static "${_IMPORT_PREFIX}/lib/libzmq-v143-mt-sgd-4_3_6.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
