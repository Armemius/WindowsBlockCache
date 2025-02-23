# Block cache lib
file(GLOB_RECURSE BLOCK_CACHE_HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/block_cache/*.hpp")
file(GLOB_RECURSE BLOCK_CACHE_LIB_SRC CONFIGURE_DEPENDS "block_cache/*.cpp")
add_library(block_cache SHARED ${BLOCK_CACHE_LIB_SRC} ${BLOCK_CACHE_HEADER_LIST})
target_include_directories(block_cache PUBLIC "${PROJECT_SOURCE_DIR}/include")
