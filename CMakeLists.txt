cmake_minimum_required(VERSION 3.15)

project(
    WindowsBlockDeviceCaching
    VERSION 1.0.0
    DESCRIPTION "Windows Block Device Caching"
    LANGUAGES C CXX
)

add_subdirectory(apps)
add_subdirectory(src)

include(cmake/compile_options.cmake)
include(cmake/install.cmake)
include(cmake/export_headers.cmake)
