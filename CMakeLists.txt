# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(BHFConverter LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_FMT "Build fmt library" ON)
option(USE_SQLITE3 "Build sqlite3 library" ON)

add_subdirectory(src)

set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "Disable dependency update." FORCE)
