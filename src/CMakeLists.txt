add_subdirectory(libs/snacman/snacman)

option (BUILD_tests "Build 'tests' application" true)
if(BUILD_tests)
    add_subdirectory(apps/snacman_tests/snacman_tests)
endif()
