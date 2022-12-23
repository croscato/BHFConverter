# Copyright (c) 2022 Gustavo Ribeiro Croscato
# SPDX-License-Identifier: MIT

set(WARNING common CACHE STRING "Define warning level to use during compilation. The valid values are: none, common, effcpp, all.")
set(WARN_VALUES none common effcpp all)

set_property(CACHE WARNING PROPERTY STRINGS ${WARN_VALUES})

list(FIND WARN_VALUES "${WARNING}" WARN_PARAM)

if(WARN_PARAM LESS 0)
    message(FATAL_ERROR "Invalid value to WARNING option (${WARNING}).")
endif()

function(target_configure_compiler target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target built by this project.")
    endif()

    if (c_compiler_gcc OR cxx_compiler_gcc)
        set_gcc_flags()
    elseif(c_compiler_clang OR cxx_compiler_clang)
        set_clang_flags()
    elseif(c_compiler_msvc OR cxx_compiler_msvc)
        set_msvc_flags()
    else()
        message(FATAL_ERROR "Can't determine the used compiler")
    endif()

    set_target_defines(${target})

    add_library(compiler_flags_${target} INTERFACE)

    target_compile_options(compiler_flags_${target} INTERFACE $<$<COMPILE_LANGUAGE:C>:${c_compiler_warnings}>)
    target_compile_options(compiler_flags_${target} INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${cxx_compiler_warnings}>)

    target_compile_options(compiler_flags_${target} INTERFACE $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release>>:${compiler_release}>)
    target_compile_options(compiler_flags_${target} INTERFACE $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:${compiler_debug}>)
    target_compile_options(compiler_flags_${target} INTERFACE $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:RelWithDebInfo>>:${compiler_rel_with_deb_info}>)
    target_compile_options(compiler_flags_${target} INTERFACE $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:MinSizeRel>>:${compiler_min_size_rel}>)

    target_link_options(compiler_flags_${target} INTERFACE $<$<AND:$<LINK_LANGUAGE:C,CXX>,$<CONFIG:Release>>:${linker_release}>)
    target_link_options(compiler_flags_${target} INTERFACE $<$<AND:$<LINK_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:${linker_debug}>)
    target_link_options(compiler_flags_${target} INTERFACE $<$<AND:$<LINK_LANGUAGE:C,CXX>,$<CONFIG:RelWithDebInfo>>:${linker_rel_with_deb_info}>)
    target_link_options(compiler_flags_${target} INTERFACE $<$<AND:$<LINK_LANGUAGE:C,CXX>,$<CONFIG:MinSizeRel>>:${linker_min_size_rel}>)

    target_compile_definitions(compiler_flags_${target} INTERFACE $<$<CONFIG:Release>:${defines_release}>)
    target_compile_definitions(compiler_flags_${target} INTERFACE $<$<CONFIG:Debug>:${defines_debug}>)
    target_compile_definitions(compiler_flags_${target} INTERFACE $<$<CONFIG:RelWithDebInfo>:${defines_rel_with_deb_info}>)
    target_compile_definitions(compiler_flags_${target} INTERFACE $<$<CONFIG:MinSizeRel>:${defines_min_size_rel}>)

    target_link_libraries(${target} PRIVATE compiler_flags_${target})
endfunction()

function(reset_flags)
    set(FLAGS
        CMAKE_EXE_LINKER_FLAGS_INIT
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_EXE_LINKER_FLAGS_RELEASE
        CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT
        CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_EXE_LINKER_FLAGS_MINSIZEREL
        CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT
        CMAKE_EXE_LINKER_FLAGS_DEBUG
        CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT
        CMAKE_SHARED_LINKER_FLAGS_INIT
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS_RELEASE
        CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT
        CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL
        CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL_INIT
        CMAKE_SHARED_LINKER_FLAGS_DEBUG
        CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT
        CMAKE_STATIC_LINKER_FLAGS_INIT
        CMAKE_STATIC_LINKER_FLAGS
        CMAKE_STATIC_LINKER_FLAGS_RELEASE
        CMAKE_STATIC_LINKER_FLAGS_RELEASE_INIT
        CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL
        CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL_INIT
        CMAKE_STATIC_LINKER_FLAGS_DEBUG
        CMAKE_STATIC_LINKER_FLAGS_DEBUG_INIT
        CMAKE_C_FLAGS_INIT
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELEASE_INIT
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_MINSIZEREL_INIT
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_DEBUG_INIT
        CMAKE_CXX_FLAGS_INIT
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELEASE_INIT
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_MINSIZEREL_INIT
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_DEBUG_INIT
    )

    set(REP_STR "(-O[0123s]|-g[0123]?|/W[1234])( |$)")

    foreach(FLAG IN LISTS FLAGS)
        string(REGEX REPLACE ${REP_STR} "" NEW_${FLAG} "${${FLAG}}")

        set(${FLAG} ${NEW_${FLAG}} PARENT_SCOPE)
    endforeach()
endfunction()

function(check_compiler)
    if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "DJGPP")
        set(c_compiler_gcc TRUE PARENT_SCOPE)
    else()
        set(c_compiler_gcc FALSE PARENT_SCOPE)
    endif()

    if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        set(c_compiler_clang TRUE PARENT_SCOPE)
    else()
        set(c_compiler_clang FALSE PARENT_SCOPE)
    endif()

    if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        set(c_compiler_msvc TRUE PARENT_SCOPE)
    else()
        set(c_compiler_msvc FALSE PARENT_SCOPE)
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "DJGPP")
        set(cxx_compiler_gcc TRUE PARENT_SCOPE)
    else()
        set(cxx_compiler_gcc FALSE PARENT_SCOPE)
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(cxx_compiler_clang TRUE PARENT_SCOPE)
    else()
        set(cxx_compiler_clang FALSE PARENT_SCOPE)
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(cxx_compiler_msvc TRUE PARENT_SCOPE)
    else()
        set(cxx_compiler_msvc FALSE PARENT_SCOPE)
    endif()
endfunction()

function (set_gcc_flags)
    get_compiler_version()

    set(compiler_version ${CMAKE_C_COMPILER_VERSION})
    set(compiler_version ${CMAKE_CXX_COMPILER_VERSION})

    if("${WARNING}" STREQUAL "none")
        list(APPEND warnings -w)
    else()
        if("${WARNING}" STREQUAL "common" OR "${WARNING}" STREQUAL "effcpp" OR "${WARNING}" STREQUAL "all")
            list(APPEND compiler_warnings
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
                -Wunused
                -Wcast-align
                -Wlogical-op
                -Wformat=2
                -Wimplicit-fallthrough
                -Wcast-qual
                -Wwrite-strings
            )

            if(compiler_version VERSION_GREATER_EQUAL 4.3)
                list(APPEND compiler_warnings -Wsign-conversion)
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 4.6)
                list(APPEND compiler_warnings -Wdouble-promotion)
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 4.8)
                list(APPEND cxx_compiler_warnings -Wuseless-cast)
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 5.0)
                list(APPEND cxx_compiler_warnings
                    -Wzero-as-null-pointer-constant
                    -Wsized-deallocation
                )
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 6.0)
                list(APPEND compiler_warnings
                    -Wmisleading-indentation
                    -Wduplicated-cond
                    -Wnull-dereference
                    -Wshift-overflow=2
                )
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 7.0)
                list(APPEND compiler_warnings -Wduplicated-branches)
            endif()

            list(APPEND cxx_compiler_warnings
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Woverloaded-virtual
                -Wctor-dtor-privacy
            )
        endif()

        if("${WARNING}" STREQUAL "effcpp" OR "${WARNING}" STREQUAL "all")
            list(APPEND cxx_compiler_warnings -Weffc++)
        endif()

        if("${WARNING}" STREQUAL "all")
            list(APPEND compiler_warnings
                -Wredundant-decls
                -Wpointer-arith
                -Wmissing-include-dirs
                -Wdisabled-optimization
                -Winvalid-pch
                -Wswitch-enum
                -Wundef
            )
        endif()
    endif()

    list(APPEND compiler_release -O2)
    list(APPEND compiler_debug -O0 -g3)
    list(APPEND compiler_rel_with_deb_info -02 -g1)
    list(APPEND compiler_min_size_rel -Os)

    list(APPEND linker_release -s)
    list(APPEND linker_debug)
    list(APPEND linker_rel_with_deb_info)
    list(APPEND linker_min_size_rel)

    list(PREPEND c_compiler_warnings ${compiler_warnings})
    list(PREPEND cxx_compiler_warnings ${compiler_warnings})

    set(c_compiler_warnings ${c_compiler_warnings} PARENT_SCOPE)
    set(cxx_compiler_warnings ${cxx_compiler_warnings} PARENT_SCOPE)

    set(c_compiler_warnings ${c_compiler_warnings} PARENT_SCOPE)
    set(cxx_compiler_warnings ${cxx_compiler_warnings} PARENT_SCOPE)

    set(compiler_release ${compiler_release} PARENT_SCOPE)
    set(compiler_debug ${compiler_debug} PARENT_SCOPE)
    set(compiler_rel_with_deb_info ${compiler_rel_with_deb_info} PARENT_SCOPE)
    set(compiler_min_size_rel ${compiler_min_size_rel} PARENT_SCOPE)

    set(linker_release ${linker_release} PARENT_SCOPE)
    set(linker_debug ${linker_debug} PARENT_SCOPE)
    set(linker_rel_with_deb_info ${linker_rel_with_deb_info} PARENT_SCOPE)
    set(linker_min_size_rel ${linker_min_size_rel} PARENT_SCOPE)
endfunction()

function (set_clang_flags)
    get_compiler_version()

    if("${WARNING}" STREQUAL "none")
        list(APPEND compiler_warnings -w)
    elseif("${WARNING}" STREQUAL "all")
        list(APPEND compiler_warnings
            -Weverything
            -Wno-c++98-compat
        )
    else()
        if("${WARNING}" STREQUAL "common" OR "${WARNING}" STREQUAL "effcpp" OR "${WARNING}" STREQUAL "all")
            list(APPEND compiler_warnings
                -Wall
                -Wextra
                -Wshadow
                -Wunused
                -Wcast-align
                -Wconversion
                -Wformat=2
                -Wimplicit-fallthrough
                -Wcast-qual
                -Wwrite-strings
                -Wsign-conversion
            )

            if(compiler_version VERSION_GREATER_EQUAL 3.2)
                list(APPEND compiler_warnings -Wpedantic)
            endif()

            if(compiler_version VERSION_GREATER_EQUAL 3.8)
                list(APPEND compiler_warnings -Wdouble-promotion)
            endif()
        endif()

        if("${WARNING}" STREQUAL "effcpp" OR "${WARNING}" STREQUAL "all")
            list(APPEND cxx_compiler_warnings -Weffc++)
        endif()
    endif()

    list(APPEND compiler_release -O2)
    list(APPEND compiler_debug -O0 -g3)
    list(APPEND compiler_rel_with_deb_info -02 -g1)
    list(APPEND compiler_min_size_rel -Os)

    list(APPEND linker_release -s)
    list(APPEND linker_debug)
    list(APPEND linker_rel_with_deb_info)
    list(APPEND linker_min_size_rel)

    list(PREPEND c_compiler_warnings ${compiler_warnings})
    list(PREPEND cxx_compiler_warnings ${compiler_warnings})

    set(c_compiler_warnings ${c_compiler_warnings} PARENT_SCOPE)
    set(cxx_compiler_warnings ${cxx_compiler_warnings} PARENT_SCOPE)

    set(compiler_release ${compiler_release} PARENT_SCOPE)
    set(compiler_debug ${compiler_debug} PARENT_SCOPE)
    set(compiler_rel_with_deb_info ${compiler_rel_with_deb_info} PARENT_SCOPE)
    set(compiler_min_size_rel ${compiler_min_size_rel} PARENT_SCOPE)

    set(linker_release ${linker_release} PARENT_SCOPE)
    set(linker_debug ${linker_debug} PARENT_SCOPE)
    set(linker_rel_with_deb_info ${linker_rel_with_deb_info} PARENT_SCOPE)
    set(linker_min_size_rel ${linker_min_size_rel} PARENT_SCOPE)
endfunction()

function (set_msvc_flags)
    if("${WARNING}" STREQUAL "none")
        list(APPEND compiler_warnings
            /w
        )
    else()
        list(APPEND compiler_warnings
            /W4     # All reasonable warnings
            /w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
            /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
            /w14263 # 'function': member function does not override any base class virtual member function
            /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
            /w14287 # 'operator': unsigned/negative constant mismatch
            /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
            /w14296 # 'operator': expression is always 'boolean_value'
            /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
            /w14545 # expression before comma evaluates to a function which is missing an argument list
            /w14546 # function call before comma missing argument list
            /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
            /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
            /w14555 # expression has no effect; expected expression with side-effect
            /w14619 # pragma warning: there is no warning number 'number'
            /w14640 # Enable warning on thread unsafe static member initialization
            /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
            /w14905 # wide string literal cast to 'LPSTR'
            /w14906 # string literal cast to 'LPWSTR'
            /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        )
    endif()

    list(APPEND compiler_release)
    list(APPEND compiler_debug)
    list(APPEND compiler_rel_with_deb_info)
    list(APPEND compiler_min_size_rel)

    list(APPEND linker_release)
    list(APPEND linker_debug)
    list(APPEND linker_rel_with_deb_info)
    list(APPEND linker_min_size_rel)

    list(PREPEND c_compiler_warnings ${compiler_warnings})
    list(PREPEND cxx_compiler_warnings ${compiler_warnings})

    set(c_compiler_warnings ${c_compiler_warnings} PARENT_SCOPE)
    set(cxx_compiler_warnings ${cxx_compiler_warnings} PARENT_SCOPE)

    set(compiler_release ${compiler_release} PARENT_SCOPE)
    set(compiler_debug ${compiler_debug} PARENT_SCOPE)
    set(compiler_rel_with_deb_info ${compiler_rel_with_deb_info} PARENT_SCOPE)
    set(compiler_min_size_rel ${compiler_min_size_rel} PARENT_SCOPE)

    set(linker_release ${linker_release} PARENT_SCOPE)
    set(linker_debug ${linker_debug} PARENT_SCOPE)
    set(linker_rel_with_deb_info ${linker_rel_with_deb_info} PARENT_SCOPE)
    set(linker_min_size_rel ${linker_min_size_rel} PARENT_SCOPE)
endfunction()

function(set_target_defines target)
    string(TOUPPER ${target} target)

    if (c_compiler_gcc OR cxx_compiler_gcc)
        list(APPEND defines "${target}_COMPILER_GCC")
    elseif(c_compiler_clang OR cxx_compiler_clang)
        list(APPEND defines "${target}_COMPILER_CLANG")
    elseif(c_compiler_msvc OR cxx_compiler_msvc)
        list(APPEND defines "${target}_COMPILER_MSVC")
    endif()

    list(APPEND defines_release "${target}_RELEASE")
    list(APPEND defines_debug "${target}_DEBUG")
    list(APPEND defines_rel_with_deb_info "${target}_REL_WITH_DEB_INFO")
    list(APPEND defines_min_size_rel "${target}_MIN_SIZE_REL")

    list(PREPEND defines_release ${defines})
    list(PREPEND defines_debug  ${defines})
    list(PREPEND defines_rel_with_deb_info  ${defines})
    list(PREPEND defines_min_size_rel  ${defines})

    set(defines_release ${defines_release} PARENT_SCOPE)
    set(defines_debug ${defines_debug} PARENT_SCOPE)
    set(defines_rel_with_deb_info ${defines_rel_with_deb_info} PARENT_SCOPE)
    set(defines_min_size_rel ${defines_min_size_rel} PARENT_SCOPE)
endfunction()

function(get_compiler_version)
    if(CMAKE_C_COMPILER_VERSION)
        set(compiler_version ${CMAKE_C_COMPILER_VERSION} PARENT_SCOPE)
    elseif(CMAKE_CXX_COMPILER_VERSION)
        set(compiler_version ${CMAKE_CXX_COMPILER_VERSION} PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Can't determine the compiler version.")
    endif()
endfunction()

reset_flags()
check_compiler()
