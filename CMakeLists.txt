cmake_minimum_required(VERSION 3.15)
project(KaboomNshit CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(KaboomNshit ${GAME_SOURCES})
target_include_directories(KaboomNshit PRIVATE src)
target_link_libraries(KaboomNshit PRIVATE raylib)

if(WIN32)
    target_link_libraries(KaboomNshit PRIVATE winmm)
endif()
