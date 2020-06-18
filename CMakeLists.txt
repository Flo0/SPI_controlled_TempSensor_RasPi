cmake_minimum_required(VERSION 3.13)
project(untitled C)

set(CMAKE_C_STANDARD 99)

file(GLOB SOURCES *.h *.c)

add_executable(embedded_1 ${SOURCE_FILES} ${SOURCES})
