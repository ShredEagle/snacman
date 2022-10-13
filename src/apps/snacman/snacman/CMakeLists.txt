set(TARGET_NAME snacman)

set(${TARGET_NAME}_HEADERS
    Entities.h
    GraphicState.h
    Logging.h
    Timing.h

    detail/Logger.h
)

set(${TARGET_NAME}_SOURCES
    main.cpp

    Entities.cpp

    detail/Logger.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS} ${${TARGET_NAME}_SOURCES}
)

add_executable(${TARGET_NAME}
               ${${TARGET_NAME}_SOURCES}
               ${${TARGET_NAME}_HEADERS}
)

cmc_target_current_include_directory(${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

cmc_cpp_all_warnings_as_errors(${TARGET_NAME} ENABLED ${BUILD_CONF_WarningAsError})

cmc_cpp_sanitizer(${TARGET_NAME} ${BUILD_CONF_Sanitizer})


##
## Dependencies
##

find_package(Math CONFIG REQUIRED math)

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        ad::math

        spdlog::spdlog
)


##
## Install
##

install(TARGETS ${TARGET_NAME})
