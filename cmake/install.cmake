if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Prefix" FORCE)
endif()

if(WIN32)
    install(FILES "$<TARGET_FILE:block_cache>" DESTINATION bin)
    install(FILES "$<TARGET_LINKER_FILE:block_cache>" DESTINATION lib)
    install(FILES "${CMAKE_BINARY_DIR}/include/block_cache/block_cache_export.h"
        DESTINATION include/block_cache)
else()
    install(TARGETS block_cache
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

if(EXISTS "${CMAKE_BINARY_DIR}/cmake")
    install(DIRECTORY "${CMAKE_BINARY_DIR}/cmake"
        DESTINATION lib/cmake/BlockCache
        FILES_MATCHING PATTERN "*Config*.cmake")
endif()
