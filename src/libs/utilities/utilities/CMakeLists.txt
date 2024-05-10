# TODO might move to handy
set(TARGET_NAME utilities)

set(${TARGET_NAME}_HEADERS
    Time.h
)

set(${TARGET_NAME}_SOURCES
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

add_library(${TARGET_NAME} INTERFACE
            ${${TARGET_NAME}_SOURCES}
            ${${TARGET_NAME}_HEADERS}
)

add_library(ad::${TARGET_NAME} ALIAS ${TARGET_NAME})

#if(MSVC)
#    # Otherwise big object files might error with:
#    # "fatal  error C1128: number of sections exceeded object file format limit: compile with /bigobj"
#    target_compile_options(${TARGET_NAME} PRIVATE "/bigobj")
#endif()

cmc_target_current_include_directory(${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES
                      LINKER_LANGUAGE CXX,
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

#cmc_cpp_all_warnings_as_errors(${TARGET_NAME} ENABLED ${BUILD_CONF_WarningAsError})


##
## Dependencies
##


##
## Install
##

install(TARGETS ${TARGET_NAME})
