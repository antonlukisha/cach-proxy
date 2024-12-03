cmake_minimum_required(VERSION 3.10)

project(ProxyServer C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

include_directories(${CMAKE_SOURCE_DIR}/includes)

file(GLOB SOURCES src/*.c)

add_executable(main ${SOURCES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
