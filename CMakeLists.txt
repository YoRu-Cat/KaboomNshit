cmake_minimum_required(VERSION 3.15)
project(KaboomNshit CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(KaboomNshit src/main.cpp)
target_link_libraries(KaboomNshit PRIVATE raylib)

if(WIN32)
    target_link_libraries(KaboomNshit PRIVATE winmm)
endif()
