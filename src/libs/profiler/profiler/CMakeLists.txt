set(TARGET_NAME profiler)

set(${TARGET_NAME}_HEADERS
    GlApi.h
    Logging.h
    Logging-init.h
    Profiler.h

    providers/ProviderApi.h
    providers/ProviderCpu.h
    providers/ProviderGL.h
    providers/ProviderRdtsc.h
)

set(${TARGET_NAME}_SOURCES
    Logging-init.cpp
    Profiler.cpp

    providers/ProviderCpu.cpp
    providers/ProviderGL.cpp
    providers/ProviderRdtsc.cpp
)


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND ${TARGET_NAME}_HEADERS
        ProfilerSeattle.h
        providers/ProviderWindows.h
    )

    list(APPEND ${TARGET_NAME}_SOURCES
        ProfilerSeattle.cpp
        providers/ProviderWindows.cpp
    )
endif()


source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

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
        ad::renderer
    PRIVATE
        ad::utilities
)


##
## Install
##

install(TARGETS ${TARGET_NAME})
