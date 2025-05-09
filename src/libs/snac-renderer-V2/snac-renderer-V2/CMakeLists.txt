set(TARGET_NAME snac-renderer-V2)

set(${TARGET_NAME}_HEADERS
    Camera.h
    Commons.h
    Constants.h
    Cube.h
    Cubemap.h
    Handle.h
    IntrospectProgram.h
    Json.h
    Lights.h
    Logging.h
    Logging-channels.h
    Logging-init.h
    Model.h
    Material.h
    Pass.h
    Profiling.h
    RendererReimplement.h
    Repositories.h
    Rigging.h
    Semantics.h
    SetupDrawing.h

    debug/DebugDrawing.h
    debug/DebugRenderer.h

    files/Dds.h
    files/BinaryArchive.h
    files/Flags.h
    files/Loader.h
    files/Versioning.h

    graph/Clipping.h
    graph/DepthMethod.h
    graph/EnvironmentMapping.h
    graph/ShadowCascadeBlock.h
    graph/ShadowMapping.h
    graph/text/Font.h
    graph/text/TextGlsl.h

    runtime_reflect/DearImguiVisitor.h
    runtime_reflect/ReflectHelpers.h

    utilities/ColorPalettes.h
    utilities/DebugDrawUtilities.h
    utilities/FrustumUtilities.h
    utilities/LoadUbos.h
    utilities/VertexStreamUtilities.h
)

set(${TARGET_NAME}_SOURCES
    Camera.cpp
    Constants.cpp
    Cubemap.cpp
    IntrospectProgram.cpp
    Logging-init.cpp
    Model.cpp
    Pass.cpp
    Rigging.cpp
    SetupDrawing.cpp

    debug/DebugDrawing.cpp
    debug/DebugRenderer.cpp

    files/Dds.cpp
    files/Loader.cpp

    graph/EnvironmentMapping.cpp
    graph/ShadowMapping.cpp
    graph/text/Font.cpp
    graph/text/TextGlsl.cpp

    utilities/LoadUbos.cpp
    utilities/VertexStreamUtilities.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

source_group(generated FILES ${CMAKE_CURRENT_BINARY_DIR}/build_info.h)

add_library(${TARGET_NAME}
            ${${TARGET_NAME}_SOURCES}
            ${${TARGET_NAME}_HEADERS}
)

add_library(ad::${TARGET_NAME} ALIAS ${TARGET_NAME})

if(MSVC)
    # Otherwise big object files might error with:
    # "fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj"
    target_compile_options(${TARGET_NAME} PRIVATE "/bigobj")
endif()

if(BUILD_SNACMAN_FEATURE_PROFILER)
    # Public, so it propagates to other targets depending on the profiler.
    target_compile_definitions(${TARGET_NAME} PUBLIC "SE_FEATURE_PROFILER")
endif()

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
        ad::resource

        #TODO Remove this! Currently needed for the debug drawer
        ad::snac-renderer-V1
        ad::reflexion
    PRIVATE
        ad::graphics
        ad::math
        ad::profiler
        ittapi::ittapi
)


##
## Install
##

install(TARGETS ${TARGET_NAME})
