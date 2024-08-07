#include "lexer.h"

auto lexer::next_token() -> token {
    auto cur = eat_next();

    while (is_space(cur)) {
        cur = eat_next();
    }

    source_location loc{source->path, line, column};

    for (auto c : singleCharTokens) {
        if (c == cur) {
            return token{loc, static_cast<token_kind>(c)};
        }
    }

    if (cur == '/' and peek() == '/') {
        while (peek() != '\n' and peek() != '\0') {
            cur = eat_next();
        }
        return next_token();
    }

    if (is_num(cur)) {
        std::string val{cur};

        while (is_num(peek())) {
            val += eat_next();
        }

        if (peek() != '.') {
            return token{loc, token_kind::Number, val};
        }
        val += eat_next();

        if (!is_num(peek())) {
            return token{loc, token_kind::Unk};
        }

        while (is_num(peek())) {
            val += eat_next();
        }

        return token{loc, token_kind::Number, val};
    }

    if (is_alpha(cur)) {
        std::string val{cur};

        while (is_alnum(peek())) {
            val += eat_next();
        }

        if (keywords.contains(val)) {
            return token{loc, keywords.at(val), val};
        }

        return token{loc, token_kind::Identifier, val};
    }

    return token{loc, token_kind::Unk};
}

auto lexer::peek() const -> char { return source->buffer[idx]; }

auto lexer::eat_next() -> char {
    column += 1;

    auto cur = peek();
    if (cur == '\n') {
        line += 1;
        column = 0;
    }

    return source->buffer[idx++];
}
