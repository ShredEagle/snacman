add_subdirectory(libs/snac-renderer/snac-renderer)

add_subdirectory(apps/snac-viewer/snac-viewer)
add_subdirectory(apps/snacman/snacman)

option (BUILD_tests "Build 'tests' application" true)
if(BUILD_tests)
    add_subdirectory(apps/snacman_tests/snacman_tests)
    add_subdirectory(apps/text_sandbox/text_sandbox)
    add_subdirectory(apps/skinning_sandbox/skinning_sandbox)
endif()
