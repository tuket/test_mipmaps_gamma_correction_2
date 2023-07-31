cmake_minimum_required(VERSION 3.11)
project(test)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(libs/glad)
add_subdirectory(libs/stb)
add_subdirectory(libs/glfw)

set(SRCS src/main.cpp)

add_executable(test ${SRCS})
target_link_libraries(test glad stb glfw)

source_group("" FILES ${SRCS})
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT test)
set_property(TARGET test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")