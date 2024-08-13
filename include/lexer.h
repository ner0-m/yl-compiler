#pragma once

#include <array>
#include <format>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "utils.h"

constexpr auto singleCharTokens = std::array{'\0', '(', ')', '{', '}', ':', ';', ',', '+', '-', '*'};

enum class token_kind : char {
    Unk = -128,

    Identifier,

    KwFn,
    KwVoid,
    KwReturn,
    KwNumber,

    Number,
    Slash,

    Eof = singleCharTokens[0],
    Lpar = singleCharTokens[1],
    Rpar = singleCharTokens[2],
    Lbrace = singleCharTokens[3],
    Rbrace = singleCharTokens[4],
    Colon = singleCharTokens[5],
    Semi = singleCharTokens[6],
    Comma = singleCharTokens[7],
    Plus = singleCharTokens[8],
    Minus = singleCharTokens[9],
    Asterisk = singleCharTokens[10],
};

auto token_kind_to_string(token_kind kind) -> std::string;

const std::unordered_map<std::string_view, token_kind> keywords = {
    {"fn", token_kind::KwFn},
    {"void", token_kind::KwVoid},
    {"return", token_kind::KwReturn},
};

struct token {
    source_location loc;
    token_kind kind;
    std::optional<std::string> value = std::nullopt;
};

template <> struct std::formatter<token> : std::formatter<std::string> {
    auto format(token t, format_context &ctx) const {
        return formatter<string>::format(std::format("Token ({}:{}:{}) {} {}", t.loc.file, t.loc.line, t.loc.col,
                                                     token_kind_to_string(t.kind), t.value.has_value() ? *t.value : ""),
                                         ctx);
    }
};

class lexer {
    const source_file *source;
    usize idx = 0;

    u64 line = 1;
    u64 column = 0;

  public:
    lexer(const source_file &source) : source(&source) {}

    auto next_token() -> token;

    auto peek() const -> char;

    auto eat_next() -> char;
};
