project(schlagwetter)

cmake_minimum_required(VERSION 2.6)

message("Welcome, friend.")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

IF (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-W -Wall -W -pedantic -std=c++0x")
  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g")
  set(CMAKE_CXX_FLAGS_PROFILE "-DDEBUG -g -pg")
  set(CMAKE_CXX_FLAGS_RELEASE "-march=native -O2 -s -DNDEBUG")
ENDIF()


IF (MSVC_IDE)
  set(CMAKE_CXX_FLAGS "/DWIN32 /D_CONSOLE /DZLIB_WINAPI /fp:fast /EHsc")
  set(CMAKE_CXX_FLAGS_DEBUG "/DDEBUG /D_DEBUG /MTd /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
ENDIF (MSVC_IDE)


# Output paths
set(EXECUTABLE_OUTPUT_PATH bin)

# Libraries

## Credits: CMake module files are taken from:
 #          - Noise: https://github.com/fador/mineserver
 #          - Readline: https://github.com/facebook/hiphop-php
 #        [ - ZLIB, Boost: expected to be part of your cmake distro ]

SET(Boost_ADDITIONAL_VERSIONS "1.42" "1.43" "1.44" "1.44" "1.45" "1.46")
find_package(Boost COMPONENTS program_options system iostreams REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

find_package(Noise REQUIRED)
include_directories(${NOISE_INCLUDE_DIRS})
set(LIBS ${LIBS} ${NOISE_LIBRARY})

find_package(Readline)
IF (READLINE_FOUND)
message("Found GNU Readline library, very good.")
include_directories(${READLINE_INCLUDE_DIRS})
set(LIBS ${LIBS} ${READLINE_LIBRARY})
IF (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_GNUREADLINE")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF (MSVC_IDE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DHAVE_GNUREADLINE")
ENDIF (MSVC_IDE)
ENDIF (READLINE_FOUND)

SET(CNBT_LIBRARY "cNBT")
include_directories(${CNBT_LIBRARY})

add_subdirectory(src)
add_subdirectory(cNBT)
