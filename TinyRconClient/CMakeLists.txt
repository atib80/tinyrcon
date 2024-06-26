# cmake_minimum_required(VERSION 3.22)
project(tinyrconclient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

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

set(src_files src/md5.cpp src/tiny_rcon_utility_functions.cpp src/connection_manager.cpp src/connection_manager_for_messages.cpp src/autoupdate.cpp src/simple_grid.cpp src/tiny_rcon_client.cpp)

add_executable(tinyrconclient ${src_files})

target_include_directories(tinyrconclient PUBLIC include deps/asio-1.26.0/include deps/bit7z-v3.2.0-msvc2019_mt_x86/bit7z/include)
target_link_directories(tinyrconclient PUBLIC deps/asio-1.26.0/include deps/bit7z-v3.2.0-msvc2019_mt_x86/bit7z/lib)
target_link_libraries(tinyrconclient PUBLIC kernel32 shell32 advapi32 user32 gdi32 comctl32 uxtheme ws2_32 bit7z.lib)
