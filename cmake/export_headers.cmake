include(GenerateExportHeader)
generate_export_header(block_cache
    EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/include/block_cache/block_cache_export.h"
    EXPORT_MACRO_NAME "BLOCK_CACHE_EXPORT"
    NO_EXPORT_MACRO_NAME "BLOCK_CACHE_NO_EXPORT"
)

target_include_directories(block_cache PUBLIC
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)