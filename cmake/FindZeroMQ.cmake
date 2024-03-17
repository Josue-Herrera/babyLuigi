# - Try to find ZMQ
# Once done this will define
# ZMQ_FOUND - System has ZMQ
# ZMQ_INCLUDE_DIRS - The ZMQ include directories
# ZMQ_LIBRARIES - The libraries needed to use ZMQ
# ZMQ_DEFINITIONS - Compiler switches required for using ZMQ

if(WIN32)
    find_path ( ZMQ_INCLUDE_DIR
        NAMES "zmq.h"
        PATHS "C:/Program Files/ZeroMQ/include")

    find_library ( ZMQ_LIBRARY
        NAMES libzmq-v143-mt-gd-4_3_6
        PATHS "C:/Program Files/ZeroMQ/lib")

    find_path ( ZMQ_CMAKE_MODULE_PATH 
        NAMES "ZeroMQConfig.cmake"
        PATHS "C:/Program Files/ZeroMQ/CMake")
        
        list(APPEND CMAKE_MODULE_PATH ${ZMQ_CMAKE_MODULE_PATH})
        set(_IMPORT_PREFIX "C:/Program Files/ZeroMQ/" CACHE STRING "IMPORT FOR ZTARGS")

        include(${ZMQ_CMAKE_MODULE_PATH}/ZeroMQConfig.cmake)
        include(${ZMQ_CMAKE_MODULE_PATH}/ZeroMQConfigVersion.cmake)
        include(${ZMQ_CMAKE_MODULE_PATH}/ZeroMQTargets.cmake)
        include(${ZMQ_CMAKE_MODULE_PATH}/ZeroMQTargets-debug.cmake)
        include(${ZMQ_CMAKE_MODULE_PATH}/ZeroMQTargets-release.cmake)

        unset(_IMPORT_PREFIX CACHE)

else()
    find_path ( ZMQ_INCLUDE_DIR
        NAMES "zmq.h"
        PATHS /opt/homebrew/Cellar/zeromq/4.3.5_1/)

    find_library ( ZMQ_LIBRARY 
        NAMES libzmq 
        PATHS /opt/homebrew/Cellar/zeromq/4.3.5_1/)
endif()

set ( ZMQ_LIBRARIES ${ZMQ_LIBRARY} )
set ( ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR} )

message(STATUS "zmq library found: ${ZMQ_LIBRARY}")

include ( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args ( ZeroMQ DEFAULT_MSG ZMQ_LIBRARY ZMQ_INCLUDE_DIR )
