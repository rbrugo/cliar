######################################################################
# @author      : rbrugo
# @file        : CMakeLists
# @created     : Tuesday Aug 27, 2024 18:49:47 CET
######################################################################

# TO USE CONAN:
#  conan install conanfile.txt --build=missing --settings=build_type=Release
#  cmake --preset conan-release
#  cmake --build --preset conan-release

cmake_minimum_required(VERSION 3.22.2)

project(cliar VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#                                Conan                                 #
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

find_package(fmt REQUIRED)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#                           Enable warnings                            #
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_library(project_warnings INTERFACE)
include(cmake/compiler_warnings.cmake)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#                                 cliar                                  #
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_library(cliar INTERFACE)
target_compile_features(cliar INTERFACE cxx_std_23)
target_compile_definitions(cliar INTERFACE NTEST)
target_link_libraries(cliar INTERFACE fmt::fmt project_warnings)
target_include_directories(cliar
    INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/include"
    SYSTEM
        "${CMAKE_CURRENT_LIST_DIR}/include/third_party"
)


option(ENABLE_TESTS "Enable tests for cliar" ON)
if (ENABLE_TESTS)
    add_executable(cliar_tests)
    target_sources(cliar_tests PUBLIC test/main.cpp)
    target_link_libraries(cliar_tests PUBLIC cliar)
endif()
