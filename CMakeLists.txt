#
#  Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)

project(fluid VERSION 0.0.0)

set(PROJECT_YEAR 2020)
set(PROJECT_URL https://github.com/cbsiddharth/fluid)
set(PROJECT_DESCRIPTION "Liquid HTML template renderer written in C for embedded devices")

## Includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(AddCCompilerFlag)
include(GitInfo)
include(BuildType)

## Global settings
add_c_compiler_flag(-Wall)
add_c_compiler_flag(-Wextra)

## Exnternal dependencies
find_package(LibYAML REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# FIXME: C-Utils must set these in parent scope if included as a
# sub direcrotry/
set(LIBUTILS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/utils/include)
set(LIBUTILS_LIBRARIES utils)

add_subdirectory(utils)
add_subdirectory(src)

## uninstall target
add_custom_target(uninstall
    COMMAND xargs rm < ${CMAKE_BINARY_DIR}/install_manifest.txt
)
