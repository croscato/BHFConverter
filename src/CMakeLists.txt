# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

string(TOLOWER ${CMAKE_PROJECT_NAME} target)

include(compiler)
include(dependencies)

set(headers
    defs.hpp
)

set(sources
    main.cpp
)

add_executable(${target} ${sources} ${headers})

set_target_properties(${target} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS ON

    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS ON

    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_configure_compiler(${target})

target_compile_options(${target} PRIVATE
    -include ${CMAKE_SOURCE_DIR}/src/defs.hpp
    -Wpadded
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(${target} PRIVATE
        -fsanitize=address
        -fsanitize=leak
        -fsanitize=undefined
        -fsanitize=bounds
        -fsanitize=bounds-strict
        -fsanitize=alignment
        -fsanitize=object-size
        -fsanitize=float-cast-overflow
        -fsanitize=nonnull-attribute
        -fsanitize=returns-nonnull-attribute
        -fsanitize=enum
        -fsanitize=vptr
        -fsanitize=pointer-overflow
        -fsanitize=builtin
        -fstack-protector-all
        -fstack-protector-strong
    )
endif()

target_link_options(${target} PRIVATE
    -fstack-protector-all
    -fstack-protector-strong
)

if(USE_FMT)
    target_link_libraries(${target} PRIVATE fmt)
endif()

if(USE_SQLITE3)
    target_link_libraries(${target} PRIVATE SQLite3::Static)
endif()

add_custom_target(run COMMAND ${target} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
