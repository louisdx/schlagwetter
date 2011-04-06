project(minerd)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)

IF (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-W -Wall -W -pedantic -std=c++0x")
  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g")
  set(CMAKE_CXX_FLAGS_RELEASE "-march=native -O2 -s -DNDEBUG")
ENDIF()


IF (MSVC_IDE)
  set(CMAKE_CXX_FLAGS "/DWIN32 /D_CONSOLE /DZLIB_WINAPI /fp:fast /EHsc")
  set(CMAKE_CXX_FLAGS_DEBUG "/DDEBUG /D_DEBUG /MTd /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
ENDIF (MSVC_IDE)


# Output paths
set(EXECUTABLE_OUTPUT_PATH bin)


add_subdirectory(src)

