set(TARGET_NAME scene-renderer)

set(${TARGET_NAME}_HEADERS
    Model.h
)

set(${TARGET_NAME}_SOURCES

)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

add_library(${TARGET_NAME}
               ${${TARGET_NAME}_SOURCES}
               ${${TARGET_NAME}_HEADERS}
)

add_library(ad::${TARGET_NAME} ALIAS ${TARGET_NAME})

cmc_target_current_include_directory(${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

cmc_cpp_all_warnings_as_errors(${TARGET_NAME} ENABLED ${BUILD_CONF_WarningAsError})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # This library includes nvpro and nvtx code which includes Windows.h in turn
    # We ensures this cannot lead to problems with min and max symbols.
    target_compile_definitions(${TARGET_NAME} PUBLIC NOMINMAX)
    # We use istrstream to interface an array of std::byte with an istream based API.
    target_compile_definitions(${TARGET_NAME} PRIVATE _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING)
else()
    target_compile_options(${TARGET_NAME} PRIVATE "-Wno-deprecated")
endif ()

cmc_cpp_sanitizer(${TARGET_NAME} ${BUILD_CONF_Sanitizer})


##
## Dependencies
##

cmc_find_dependencies()

# Add NVTX Imported CMake targets
#include("3rdparty/nvtx/nvtxImportedTargets.cmake")

target_link_libraries(${TARGET_NAME}
    PUBLIC
        ad::math
#        nvtx3-cpp
    PRIVATE
#        ad::arte
#        ad::graphics
#        ad::imguiui 
        ad::renderer
#        ad::resource

        spdlog::spdlog
)

# With this definition, the nvpro Profiler will generate nvtx events/ranges.
target_compile_definitions(${TARGET_NAME} PUBLIC NVP_SUPPORTS_NVTOOLSEXT)

##
## Install
##

install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME}Targets)

include(cmc-install)
# Install the header files
cmc_install_with_folders(DESTINATION include/${TARGET_NAME}/${TARGET_NAME}
                         FILES ${${TARGET_NAME}_HEADERS})
# Prepare CMake package in both build and install trees
cmc_install_packageconfig(${TARGET_NAME} ${TARGET_NAME}Targets ${PROJECT_NAME}
                          FIND_FILE CMakeFinds.cmake.in
                          NAMESPACE ad::)
