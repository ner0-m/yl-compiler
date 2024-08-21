#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

using f32 = float;
using f64 = double;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::ptrdiff_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;

struct source_file {
    std::string_view path;
    std::string buffer;
};

struct source_location {
    std::string_view file;
    u64 line;
    u64 col;
};

template <> struct std::formatter<source_location> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const source_location &loc, std::format_context &ctx) const {
        return std::format_to(ctx.out(), "{}:{}:{}", loc.file, loc.line, loc.col);
    }
};

auto is_space(i8 c) -> bool;

auto is_alpha(i8 c) -> bool;

auto is_num(char c) -> bool;

auto is_alnum(char c) -> bool;

auto indent(size_t level) -> std::string;

std::nullptr_t report(source_location location, std::string_view message, bool isWarning = false);
