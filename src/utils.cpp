#include <iostream>

#include "utils.h"

auto is_space(i8 c) -> bool { return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'; }

auto is_alpha(i8 c) -> bool { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

auto is_num(char c) -> bool { return '0' <= c && c <= '9'; }

auto is_alnum(char c) -> bool { return is_alpha(c) || is_num(c); }

std::string indent(size_t level) { return std::string(level * 2, ' '); }

std::nullptr_t report(source_location location, std::string_view message, bool isWarning) {
    const auto &[file, line, col] = location;

    std::print(std::cerr, "{}:{}:{}: {}: {}\n", file, line, col, (isWarning ? "warning" : "error"), message);

    return nullptr;
}
