set(TARGET_NAME snac-reflexion)

set(${TARGET_NAME}_HEADERS
  Reflexion.h
  Reflexion_impl.h
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}
             FILES ${${TARGET_NAME}_HEADERS}
)

add_library(${TARGET_NAME} INTERFACE
)

add_library(ad::${TARGET_NAME} ALIAS ${TARGET_NAME})

cmc_target_current_include_directory(${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES
                      VERSION "${${PROJECT_NAME}_VERSION}"
)

##
## Dependencies
##

cmc_find_dependencies()

target_link_libraries(${TARGET_NAME}
    INTERFACE
        ad::handy
        ad::entity
        nlohmann_json::nlohmann_json
)


##
## Install
##

install(TARGETS ${TARGET_NAME})
