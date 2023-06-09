cmake_minimum_required(VERSION 3.22)
project(tinyrcon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

if (MSVC)
    add_compile_options(/nologo /std:c++20 /W4 /EHsc /O2 /Ot /Ox /Ob2 /Oy /Oi)
else ()
    add_compile_options(-std=c++20 -Wall -Wextra -Wpedantic -mwindows -O3 -Ofast)
    # add_compile_options(-std=c++20 -Wall -Wextra -Wpedantic -mwindows -m64 -O3 -Ofast -finline-functions)
endif ()

# Tell CMake to run moc when necessary:
# set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Widgets finds its own dependencies.
# find_package(Qt5Core REQUIRED)

# find_package(Qt6 REQUIRED COMPONENTS Core)
# qt_standard_project_setup()

#set(BOOST_REQUESTED_VERSION 1.80)
#set(BOOST_ROOT_DIR build)
#list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Add Boost Lib Sources
set(BOOST_INCLUDE_LIBRARIES asio system)
set(BOOST_ENABLE_CMAKE ON)

# Download and Extract the Boost Library from GitHub
# message(STATUS "Downloading and extracting Boost Library Sources. This will take some time...")
# include(FetchContent)
# Set(FETCHCONTENT_QUIET FALSE) # Needed to print downloading progress
# FetchContent_Declare(
#       Boost
#       URL https://github.com/boostorg/boost/releases/download/boost-1.81.0/boost-1.81.0.7z # downloading a zip release speeds up the download
#       USES_TERMINAL_DOWNLOAD TRUE
#       GIT_PROGRESS TRUE
#       DOWNLOAD_NO_EXTRACT FALSE
#)
# FetchContent_MakeAvailable(Boost)

set(src_files src/simple_grid.cpp src/tiny_rcon_utility_functions.cpp src/connection_manager.cpp src/tiny_rcon.cpp)

add_executable(tinyrcon ${src_files})

target_include_directories(tinyrcon PUBLIC include)
target_include_directories(tinyrcon PUBLIC deps/asio-1.26.0/include)
target_link_libraries(tinyrcon PUBLIC kernel32 shell32 advapi32 user32 gdi32 comctl32 uxtheme ws2_32)