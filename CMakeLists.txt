cmake_minimum_required(VERSION 3.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

project(ttracker)

include_directories(include)
file(GLOB_RECURSE SOURCE
    "src/*.c"
)

add_executable(tt ${SOURCE})
