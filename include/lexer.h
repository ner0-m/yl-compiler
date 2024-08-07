#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "utils.h"

constexpr auto singleCharTokens = std::array{'\0', '(', ')', '{', '}', ':', ';', ','};

enum class token_kind : char {
    Identifier,
    Unk = -128,

    KwFn,
    KwVoid,
    KwReturn,
    KwNumber,

    Number,

    Eof = singleCharTokens[0],
    Lpar = singleCharTokens[1],
    Rpar = singleCharTokens[2],
    Lbrace = singleCharTokens[3],
    Rbrace = singleCharTokens[4],
    Colon = singleCharTokens[5],
    Semi = singleCharTokens[6],
    Comma = singleCharTokens[7],
};

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
