cmake_minimum_required(VERSION 3.22)

project(TinyRcon VERSION 2.7.5.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

add_subdirectory(TinyRconClient)
add_subdirectory(TinyRconServer)