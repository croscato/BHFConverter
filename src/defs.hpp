// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Gustavo Ribeiro Croscato

#ifndef CMAKE_TEMPLATE_SRC_DEFS_HPP
#define CMAKE_TEMPLATE_SRC_DEFS_HPP 1

#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstdio>

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"

#if defined(USING_FMT)
#   include <fmt/core.h>
#   include <fmt/format.h>
#endif

#if defined(USING_SQLITE3)
#   include <sqlite3.h>
#endif

#pragma GCC diagnostic pop

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using isize = std::int64_t;
using usize = std::uint64_t;

using f32 = float;
using f64 = double;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
static_assert(sizeof(isize) == 8);
static_assert(sizeof(usize) == 8);

#define UNUSED(param) ((void)(param))

#endif // CMAKE_TEMPLATE_DEFS_HPP

