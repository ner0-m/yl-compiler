#include "lexer.h"

auto token_kind_to_string(token_kind kind) -> std::string {
    switch (kind) {
    case (token_kind::Identifier):
        return "identifier";
    case (token_kind::Unk):
        return "Unknown";
    case (token_kind::KwFn):
        return "fn";
    case (token_kind::KwVoid):
        return "void";
    case (token_kind::KwReturn):
        return "return";
    case (token_kind::KwNumber):
        return "number";
    case (token_kind::Number):
        return "<number>";
    case (token_kind::Eof):
        return "eof";
    case (token_kind::Lpar):
        return "(";
    case (token_kind::Rpar):
        return ")";
    case (token_kind::Lbrace):
        return "{";
    case (token_kind::Rbrace):
        return "}";
    case (token_kind::Colon):
        return ":";
    case (token_kind::Semi):
        return ";";
    case (token_kind::Comma):
        return ",";
    case (token_kind::Plus):
        return "+";
    case (token_kind::Minus):
        return "-";
    case (token_kind::Asterisk):
        return "*";
    case (token_kind::Slash):
        return "/";
    }
    __builtin_unreachable();
}

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

    if (cur == '/') {
        if (peek() != '/') {
            return token{loc, token_kind::Slash};
        }
        if (peek() == '/') {
            while (peek() != '\n' and peek() != '\0') {
                cur = eat_next();
            }
            return next_token();
        }
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
