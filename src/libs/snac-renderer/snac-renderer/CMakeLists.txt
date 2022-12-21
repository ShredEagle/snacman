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
    Semantic.h
    SetupDrawing.h
    ShaderResource.h
)

set(${TARGET_NAME}_SOURCES
    Camera.cpp
    Effect.cpp
    IntrospectProgram.cpp
    Logging.cpp
    Material.cpp
    Mesh.cpp
    Render.cpp
    Semantic.cpp
    SetupDrawing.cpp
    ShaderResource.cpp
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

cmc_cpp_sanitizer(${TARGET_NAME} ${BUILD_CONF_Sanitizer})


##
## Dependencies
##

cmc_find_dependencies()

target_link_libraries(${TARGET_NAME}
    PUBLIC
        ad::math
    PRIVATE
        ad::graphics
        ad::renderer

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
