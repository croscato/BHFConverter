# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

string(TOLOWER ${CMAKE_PROJECT_NAME} target)

include(compiler)
include(dependencies)
include(target)

# BHF
set(bhf_headers
    bhf/types.hpp
    bhf/file.hpp
)

set(bhf_sources
    bhf/file.cpp
)

add_library(${target}_lib OBJECT ${bhf_sources} ${bhf_headers})

configure_target(${target}_lib)

# CLI
set(cli_headers)

set(cli_sources
    cli/main.cpp
)

add_executable(${target}_cli ${cli_sources} ${cli_headers})

configure_target(${target}_cli)

target_include_directories(${target}_cli PRIVATE ${CMAKE_CURRENT_LIST_DIR}/cli)

target_link_libraries(${target}_cli PRIVATE ${target}_lib)

set_target_properties(${target}_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# GUI
set(gui_headers
    gui/ui/mainwindow.hpp
    gui/model/context.hpp
    gui/model/index.hpp
)

set(gui_sources
    gui/main.cpp
    gui/ui/mainwindow.cpp
    gui/model/context.cpp
    gui/model/index.cpp
)

find_package(Qt6 COMPONENTS Widgets)

if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    qt_add_executable(${target}_gui ${gui_sources} ${gui_headers})

    configure_target(${target}_gui)

    target_include_directories(${target}_gui PRIVATE ${CMAKE_CURRENT_LIST_DIR}/gui)

    target_compile_options(${target}_gui PRIVATE
        SHELL:-include ${CMAKE_CURRENT_LIST_DIR}/gui/defs.hpp
    )

    target_link_libraries(${target}_gui PRIVATE ${target}_lib Qt6::Widgets)

    set_target_properties(${target}_gui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Common
source_group("Headers" FILES ${bhf_headers} ${cli_headers} ${gui_headers})
source_group("Sources" FILES ${bhf_sources} ${cli_sources} ${gui_sources})
source_group("Forms" FILES ${gui_forms})
source_group("Resources" FILES ${gui_resources})
