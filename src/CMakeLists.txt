add_subdirectory(libs/utilities/utilities)
add_subdirectory(libs/profiler/profiler)
# TODO #decommissionRV1
# We decommissioned the renderer-V1 build, but we kept the folder around.
# We are sentimental like that.
add_subdirectory(libs/snac-renderer-V1/snac-renderer-V1)
add_subdirectory(libs/snac-renderer-V2/snac-renderer-V2)
add_subdirectory(libs/file-processor/file-processor)
add_subdirectory(libs/snac-reflexion/snac-reflexion)

add_subdirectory(apps/snac-viewer/snac-viewer)
add_subdirectory(apps/snacman/snacman)
add_subdirectory(apps/asset_processor/asset_processor)
#add_subdirectory(apps/serial/serial)

option (BUILD_tests "Build 'tests' application" true)
if(BUILD_tests)
    add_subdirectory(apps/viewer/viewer)
    add_subdirectory(apps/skinning_sandbox/skinning_sandbox)
    add_subdirectory(apps/snacman_tests/snacman_tests)
    add_subdirectory(apps/text_sandbox/text_sandbox)
endif()
