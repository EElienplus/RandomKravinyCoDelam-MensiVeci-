# CmakeLists.txt made with AI

cmake_minimum_required(VERSION 4.0)
project(Calculator LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

find_path(RAYLIB_INCLUDE_DIR raylib.h HINTS /opt/homebrew/include /usr/local/include)
find_library(RAYLIB_LIBRARY NAMES raylib HINTS /opt/homebrew/lib /usr/local/lib)


add_executable(Calculator main.cpp)
target_include_directories(Calculator PRIVATE ${RAYLIB_INCLUDE_DIR})
target_link_libraries(Calculator PRIVATE ${RAYLIB_LIBRARY})
