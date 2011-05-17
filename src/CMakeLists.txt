set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
  chunk.cpp
  cmdlineoptions.cpp
  connection.cpp
  constants.cpp
  filereader.cpp
  gamestatemanager.cpp
  gamestateserializer.cpp
  generator.cpp
  inputparser.cpp
  main.cpp
  map.cpp
  packethandlers.cpp
  random.cpp
  serializer.cpp
  server.cpp
  sha1.cpp
  ui.cpp
)

add_executable(schlagwetter ${SOURCES})

add_executable(nbtimporter nbtimporter.cpp filereader.cpp)

add_executable(tester_filereader tester_filereader.cpp filereader.cpp)
#add_executable(tester_packetcrafter tester_packetcrafter.cpp)

target_link_libraries(schlagwetter ${LIBS} "nbt")

target_link_libraries(tester_filereader ${ZLIB_LIBRARIES} "nbt")

target_link_libraries(nbtimporter ${LIBS} "nbt")
