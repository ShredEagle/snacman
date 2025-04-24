set(TARGET_NAME file-processor)

set(${TARGET_NAME}_HEADERS
    AssimpUtils.h
    Logging.h
    Logging-init.h
    ProcessAnimation.h
    Processor.h
)

set(${TARGET_NAME}_SOURCES
    ProcessAnimation.cpp
    Logging-init.cpp
    Processor.cpp
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
    PRIVATE
        ad::arte
        ad::math
        ad::snac-renderer-V2

        assimp::assimp
        spdlog::spdlog
)


##
## Install
##

install(TARGETS ${TARGET_NAME})
