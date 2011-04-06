set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB SOURCES *.cpp)

add_executable(minerd ${SOURCES})

target_link_libraries(minerd z noise readline boost_system boost_program_options)