# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

include(FindGit)

find_package(Git)

if (NOT Git_FOUND)
    message(FATAL_ERROR "Git not found!")
endif ()

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 9.1.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)

if(USE_FMT)
    set(FMT_DOC OFF)
    set(FMT_INSTALL OFF)
    set(FMT_TEST OFF)

    FetchContent_MakeAvailable(fmt)

    add_compile_definitions(USING_FMT)
endif()

FetchContent_Declare(sqlite3
    GIT_REPOSITORY https://github.com/sqlite/sqlite.git
    GIT_TAG version-3.40.0
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)

if(USE_SQLITE3)
    FetchContent_Populate(sqlite3)

    if(NOT sqlite3_CONFIGURED)
        set(sqlite3_CONFIGURED TRUE CACHE BOOL "SQlite3 is configured" FORCE)

        execute_process(COMMAND ./configure WORKING_DIRECTORY ${sqlite3_SOURCE_DIR})
        execute_process(COMMAND make sqlite3.c WORKING_DIRECTORY ${sqlite3_SOURCE_DIR})

        configure_file(
            ${CMAKE_SOURCE_DIR}/generated/sqlite3_cmakelists.txt.in
            ${sqlite3_SOURCE_DIR}/CMakeLists.txt
            @ONLY
        )
    endif()

    add_subdirectory(${sqlite3_SOURCE_DIR} ${sqlite3_BINARY_DIR})
endif()

