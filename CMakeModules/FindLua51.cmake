# Find the Lua 5.1 includes and library
#
# LUA51_INCLUDE_DIR - where to find lua.h
# LUA51_LIBRARIES - List of fully qualified libraries to link against
# LUA51_FOUND - Set to TRUE if found

# Copyright (c) 2007, Pau Garcia i Quiles, <pgquiles@elpauer.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF(LUA51_INCLUDE_DIR AND LUA51_LIBRARIES)
    SET(LUA51_FIND_QUIETLY TRUE)
ENDIF(LUA51_INCLUDE_DIR AND LUA51_LIBRARIES)

FIND_PATH(LUA51_INCLUDE_DIR lua5.1/lua.h )

FIND_LIBRARY(LUA51_LIBRARIES NAMES lua5.1 )

IF(LUA51_INCLUDE_DIR AND LUA51_LIBRARIES)
   SET(LUA51_FOUND TRUE)
   INCLUDE(CheckLibraryExists)
   CHECK_LIBRARY_EXISTS(${LUA51_LIBRARIES} lua_close "" LUA51_NEED_PREFIX)
ELSE(LUA51_INCLUDE_DIR AND LUA51_LIBRARIES)
   SET(LUA51_FOUND FALSE)
   MESSAGE("D'oh")
ENDIF (LUA51_INCLUDE_DIR AND LUA51_LIBRARIES)

IF(LUA51_FOUND)
  IF (NOT LUA51_FIND_QUIETLY)
    MESSAGE(STATUS "Found Lua 5.1 library: ${LUA51_LIBRARIES}")
    MESSAGE(STATUS "Found Lua 5.1 headers: ${LUA51_INCLUDE_DIR}")
  ENDIF (NOT LUA51_FIND_QUIETLY)
ELSE(LUA51_FOUND)
  IF(LUA51_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could NOT find Lua 5.1")
  ENDIF(LUA51_FIND_REQUIRED)
ENDIF(LUA51_FOUND)

MARK_AS_ADVANCED(LUA51_INCLUDE_DIR LUA51_LIBRARIES)
