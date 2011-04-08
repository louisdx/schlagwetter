set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
  cmdlineoptions.cpp
  connection.cpp
  filereader.cpp
  gamestatemanager.cpp
  generator.cpp
  inputparser.cpp
  main.cpp
  map.cpp
  packets.cpp
  server.cpp
  ui.cpp
)

add_executable(minerd ${SOURCES})

add_executable(tester_filereader tester_filereader.cpp filereader.cpp)

target_link_libraries(minerd ${LIBS})

target_link_libraries(tester_filereader ${ZLIB_LIBRARIES})
