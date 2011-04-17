project(luabind)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

IF (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-W -Wall -W -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g")
  set(CMAKE_CXX_FLAGS_PROFILE "-DDEBUG -g -pg")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s -DNDEBUG")
ENDIF()

IF (MSVC_IDE)
  set(CMAKE_CXX_FLAGS "/DWIN32 /D_CONSOLE /DZLIB_WINAPI /fp:fast /EHsc")
  set(CMAKE_CXX_FLAGS_DEBUG "/DDEBUG /D_DEBUG /MTd /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
ENDIF (MSVC_IDE)


find_package(Lua51 REQUIRED)
set(LIBS ${LIBS} ${LUA51_LIBRARIES})
include_directories(${LUA_INCLUDE_DIR})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

ADD_LIBRARY(luabind
  src/class.cpp
  src/class_info.cpp
  src/class_registry.cpp
  src/class_rep.cpp
  src/create_class.cpp
  src/error.cpp
  src/exception_handler.cpp
  src/function.cpp
  src/inheritance.cpp
  src/link_compatibility.cpp
  src/object_rep.cpp
  src/open.cpp
  src/pcall.cpp
  src/scope.cpp
  src/stack_content_by_name.cpp
  src/weak_ref.cpp
  src/wrapper_base.cpp
)
