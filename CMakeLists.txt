cmake_minimum_required(VERSION 3.23)

# Setting the VERSION on root project() will populate CMAKE_PROJECT_VERSION
# see: https://cmake.org/cmake/help/v3.13/variable/CMAKE_PROJECT_VERSION.html
project(Snacman VERSION "${BUILD_VERSION}")

# Setup cmake helpers in 'cmake' submodule
include ("cmake/include.cmake")

cmc_include_conan_configuration()

# Enable the grouping target in folders, when available in IDE.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Note: also set by conanuser_config.cmake
option (BUILD_CONF_WarningAsError "Enable warning as error compiler option." False)

option(BUILD_SNACMAN_FEATURE_PROFILER "Enable profiler feature (compile time)." ON)

# Install the top-level package config, allowing to find all sub-components
include(cmc-install)
cmc_install_root_component_config(${PROJECT_NAME})
cmc_register_source_package(${PROJECT_NAME})

include(cmc-cpp)
cmc_cpp_define_sanitizer_enum()
add_subdirectory(src)
