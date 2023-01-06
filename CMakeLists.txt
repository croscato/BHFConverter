# SPDX-License-Identifier: MIT
# Copyright (c) 2022 Gustavo Ribeiro Croscato

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(BHFConverter LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS ON)

option(USE_FMT "Build fmt library" ON)
option(USE_SQLITE3 "Build sqlite3 library" ON)

add_subdirectory(src)

set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "Disable dependency update." FORCE)
