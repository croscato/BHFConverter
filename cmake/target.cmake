# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

function(configure_target target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target built by this project.")
    endif()

    target_configure_compiler(${target})

    target_compile_options(${target} PRIVATE
        -include ${CMAKE_CURRENT_LIST_DIR}/defs.hpp
    )

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
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
endfunction()
