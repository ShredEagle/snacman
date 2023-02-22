set(TARGET_NAME snac-renderer)

set(${TARGET_NAME}_HEADERS
    Camera.h
    Cube.h
    Effect.h
    IntrospectProgram.h
    Logging.h
    Material.h
    Mesh.h
    Render.h
    ResourceLoad.h
    Semantic.h
    SetupDrawing.h
    ShaderResource.h
    UniformParameters.h

    gltf/GltfLoad.h
    gltf/LoadBuffer.h

    text/Text.h

    3rdparty/Profiler.hpp
)

set(${TARGET_NAME}_SOURCES
    Camera.cpp
    Effect.cpp
    IntrospectProgram.cpp
    Logging.cpp
    Material.cpp
    Mesh.cpp
    Render.cpp
    ResourceLoad.cpp
    Semantic.cpp
    SetupDrawing.cpp
    ShaderResource.cpp

    text/Text.cpp

    gltf/GltfLoad.cpp
    gltf/LoadBuffer.cpp

    3rdparty/Profiler.cpp
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

target_link_libraries(${TARGET_NAME}
    PUBLIC
        ad::math
    PRIVATE
        ad::arte
        ad::graphics
        ad::renderer
        ad::resource

        spdlog::spdlog
)


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
