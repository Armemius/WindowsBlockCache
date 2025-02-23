cmake_minimum_required(VERSION 3.15)

project(
    WindowsBlockDeviceCaching
    VERSION 1.0.0
    DESCRIPTION "Windows Block Device Caching"
    LANGUAGES CXX
)

include(cmake/compile_options.cmake)

add_subdirectory(apps)
add_subdirectory(src)
