#include "parser.h"

#include <print>

auto tok_precedence(token_kind kind) -> i32 {
    switch (kind) {
    case token_kind::Asterisk:
    case token_kind::Slash:
        return 6;
    case token_kind::Plus:
    case token_kind::Minus:
        return 5;
    case token_kind::Lt:
    case token_kind::Gt:
        return 4;
    case token_kind::EqualEqual:
        return 3;
    case token_kind::AmpAmp:
        return 2;
    case token_kind::PipePipe:
        return 1;
    default:
        return -1;
    }
}

auto parser::eat_next() -> void { next_token = lex->next_token(); };

auto parser::synchronize_on(token_kind kind) -> void {
    incomplete_ast = true;

    while (next_token.kind != kind and next_token.kind != token_kind::Eof) {
        eat_next();
    }
}

auto parser::synchronize() -> void {
    incomplete_ast = true;

    u64 braces = 0;
    while (true) {
        auto kind = next_token.kind;
        if (kind == token_kind::Lbrace) {
            ++braces;
        } else if (kind == token_kind::Rbrace) {
            if (braces == 0) {
                break;
            } else if (braces == 1) {
                eat_next(); // eat '}'
                break;
            }

            --braces;
        } else if (kind == token_kind::Semi && braces == 0) {
            eat_next(); // eat ';'
            break;
        } else if (kind == token_kind::KwFn || kind == token_kind::Eof) {
            break;
        }
        eat_next();
    }
}

auto parser::parse_source_file() -> std::pair<std::vector<std::unique_ptr<function_decl>>, bool> {
    std::vector<std::unique_ptr<function_decl>> functions;

    while (next_token.kind != token_kind::Eof) {
        if (next_token.kind != token_kind::KwFn) {
            report(next_token.loc, "only function declarations are allowed on the top level");

            synchronize_on(token_kind::KwFn);
            continue;
        }

        auto fn = parse_function_decl();

        if (!fn) {
            synchronize_on(token_kind::KwFn);
            continue;
        }

        functions.emplace_back(std::move(fn));
    }

    bool has_main_fn = false;
    for (auto &&fn : functions)
        has_main_fn |= fn->identifier == "main";

    if (!has_main_fn && !incomplete_ast)
        report(next_token.loc, "main function not found");

    return {std::move(functions), !incomplete_ast && has_main_fn};
}

auto parser::parse_function_decl() -> std::unique_ptr<function_decl> {
    auto loc = next_token.loc;

    eat_next();

    if (next_token.kind != token_kind::Identifier) {
        return report(next_token.loc, "expected identifier");
    }

    std::string fn_id = *next_token.value;
    eat_next(); // eat identifier

    auto param_list = parse_param_list();
    if (!param_list) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Colon) {
        return report(next_token.loc, "expected ':'");
    }
    eat_next(); // :

    auto type = parse_type();
    if (!type) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Lbrace) {
        return report(next_token.loc, "expected function body starting with '{'");
    }

    auto block = parse_block();
    if (!block) {
        return nullptr;
    }

    return std::make_unique<function_decl>(loc, fn_id, std::move(*param_list), *type, std::move(block));
}

auto parser::parse_param_list() -> std::unique_ptr<std::vector<std::unique_ptr<param_decl>>> {
    if (next_token.kind != token_kind::Lpar) {
        return report(next_token.loc, "expected '('");
    }
    eat_next(); // eat '('

    std::vector<std::unique_ptr<param_decl>> param_list;

    while (true) {
        if (next_token.kind == token_kind::Rpar) {
            break;
        }

        if (next_token.kind != token_kind::Identifier) {
            return report(next_token.loc, "expected parameter declaration");
        }

        auto param = parse_param_decl();
        if (!param) {
            return nullptr;
        }
        param_list.emplace_back(std::move(param));

        if (next_token.kind != token_kind::Comma) {
            break;
        }
        eat_next(); // eat ','
    }

    if (next_token.kind != token_kind::Rpar) {
        return report(next_token.loc, "expected ')'");
    }
    eat_next(); // eat ')'

    return std::make_unique<std::vector<std::unique_ptr<param_decl>>>(std::move(param_list));
}

auto parser::parse_arg_list() -> std::unique_ptr<std::vector<std::unique_ptr<expr>>> {
    auto loc = next_token.loc;
    if (next_token.kind != token_kind::Lpar) {
        return report(next_token.loc, "expected '('");
    }
    eat_next(); // eat '('

    std::vector<std::unique_ptr<expr>> arg_list;
    while (true) {
        if (next_token.kind == token_kind::Rpar) {
            break;
        }

        auto expr = parse_expr();
        if (!expr) {
            return nullptr;
        }
        arg_list.emplace_back(std::move(expr));

        if (next_token.kind != token_kind::Comma) {
            break;
        }
        eat_next(); // eat ','
    }

    if (next_token.kind != token_kind::Rpar) {
        return report(next_token.loc, "expected ')'");
    }
    eat_next(); // eat ')'

    return std::make_unique<std::vector<std::unique_ptr<expr>>>(std::move(arg_list));
}

auto parser::parse_type() -> std::optional<type> {
    auto kind = next_token.kind;

    if (kind == token_kind::KwVoid) {
        eat_next();
        return type::builtin_void();
    }

    if (kind == token_kind::Identifier) {
        auto t = type::custom(*next_token.value);
        eat_next();
        return t;
    }

    if (kind == token_kind::KwNumber) {
        eat_next();
        return type::builtin_number();
    }

    report(next_token.loc, "expected type specifier");
    return std::nullopt;
}

auto parser::parse_block() -> std::unique_ptr<block> {
    auto loc = next_token.loc;
    eat_next(); // eat {

    std::vector<std::unique_ptr<stmt>> stmts;
    while (true) {
        if (next_token.kind == token_kind::Rbrace) {
            break;
        }

        if (next_token.kind == token_kind::Eof or next_token.kind == token_kind::KwFn) {
            return report(next_token.loc, "expected '}' at the end of a block");
        }

        auto stmt = parse_stmt();
        if (!stmt) {
            synchronize();
            continue;
        }
        stmts.emplace_back(std::move(stmt));
    }

    if (next_token.kind != token_kind::Rbrace) {
        return report(next_token.loc, "expected '}' at the end of a block");
    }
    eat_next(); // eat '}'

    return std::make_unique<block>(loc, std::move(stmts));
}

auto parser::parse_stmt() -> std::unique_ptr<stmt> {
    if (next_token.kind == token_kind::KwIf) {
        return parse_if_stmt();
    }

    if (next_token.kind == token_kind::KwWhile) {
        return parse_while_stmt();
    }

    if (next_token.kind == token_kind::KwReturn) {
        return parse_return_stmt();
    }

    if (next_token.kind == token_kind::KwLet || next_token.kind == token_kind::KwVar) {
        return parse_decl_stmt();
    }

    auto expr = parse_expr();
    if (!expr) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Semi) {
        return report(next_token.loc, "expected ';' at the end of expression");
    }
    eat_next(); // eat ';'

    return expr;
}

auto parser::parse_decl_stmt() -> std::unique_ptr<decl_stmt> {
    auto tok = next_token;

    eat_next(); // eat 'var' or 'let'

    if (next_token.kind != token_kind::Identifier) {
        return report(next_token.loc, "expected identifier");
    }

    auto vardecl = parse_var_decl(tok.kind == token_kind::KwLet);
    if (!vardecl) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Semi) {
        return report(next_token.loc, "expected ';' after declaration");
    }
    eat_next(); // eat ';'

    return std::make_unique<decl_stmt>(tok.loc, std::move(vardecl));
}

auto parser::parse_var_decl(bool is_let) -> std::unique_ptr<var_decl> {
    auto loc = next_token.loc;

    auto id = *next_token.value;
    eat_next(); // eat identifier

    std::optional<type> t;
    if (next_token.kind == token_kind::Colon) {
        eat_next(); // eat ':'

        t = parse_type();
        if (!t.has_value()) {
            return nullptr;
        }
    }

    if (next_token.kind != token_kind::Equal) {
        return std::make_unique<var_decl>(loc, id, t, !is_let);
    }

    eat_next(); // eat '='

    auto init = parse_expr();
    if(!init) {
        return nullptr;
    }
    return std::make_unique<var_decl>(loc, id, t, !is_let, std::move(init));
}

auto parser::parse_if_stmt() -> std::unique_ptr<if_stmt> {
    auto loc = next_token.loc;
    eat_next(); // eat 'if'

    auto condition = parse_expr();
    if (!condition) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Lbrace) {
        return report(next_token.loc, "expected 'if' body");
    }

    auto true_block = parse_block();
    if (!true_block) {
        return nullptr;
    }

    if (next_token.kind != token_kind::KwElse) {
        return std::make_unique<if_stmt>(loc, std::move(condition), std::move(true_block));
    }

    eat_next();

    std::unique_ptr<block> false_block;
    if (next_token.kind == token_kind::KwIf) {
        auto else_if = parse_if_stmt();
        if (!else_if) {
            return nullptr;
        }

        auto else_if_loc = else_if->loc;
        std::vector<std::unique_ptr<stmt>> stmts;
        stmts.emplace_back(std::move(else_if));
        false_block = std::make_unique<block>(loc, std::move(stmts));
    } else {
        if (next_token.kind != token_kind::Lbrace) {
            return report(next_token.loc, "expected 'else' body");
        }
        false_block = parse_block();
    }

    if (!false_block) {
        return nullptr;
    }

    return std::make_unique<if_stmt>(loc, std::move(condition), std::move(true_block), std::move(false_block));
}

auto parser::parse_while_stmt() -> std::unique_ptr<while_stmt> {
    auto loc = next_token.loc;
    eat_next(); // eat 'while'

    auto cond = parse_expr();
    if (!cond) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Lbrace) {
        return report(next_token.loc, "expected 'while' body");
    }

    auto body = parse_block();
    if (!body) {
        return nullptr;
    }
    return std::make_unique<while_stmt>(loc, std::move(cond), std::move(body));
}

auto parser::parse_expr() -> std::unique_ptr<expr> {
    auto lhs = parse_prefix_expr();
    if (!lhs) {
        return nullptr;
    }

    return parse_expr_rhs(std::move(lhs), 0);
}

auto parser::parse_expr_rhs(std::unique_ptr<expr> lhs, i32 precedence) -> std::unique_ptr<expr> {
    while (true) {
        auto op = next_token;
        auto cur_precedence = tok_precedence(op.kind);

        if (cur_precedence < precedence) {
            return lhs;
        }

        eat_next(); // eat operator

        auto rhs = parse_prefix_expr();
        if (!rhs) {
            return nullptr;
        }

        if (cur_precedence < tok_precedence(next_token.kind)) {
            rhs = parse_expr_rhs(std::move(rhs), cur_precedence + 1);
            if (!rhs) {
                return nullptr;
            }
        }

        lhs = std::make_unique<binary_op>(op.loc, std::move(lhs), std::move(rhs), op.kind);
    }
}

auto parser::parse_return_stmt() -> std::unique_ptr<return_stmt> {
    auto loc = next_token.loc;
    eat_next(); // eat "return"

    std::unique_ptr<expr> e;
    if (next_token.kind != token_kind::Semi) {
        e = parse_expr();

        if (!e) {
            return nullptr;
        }
    }

    if (next_token.kind != token_kind::Semi) {
        return report(next_token.loc, "expected ';' at the end of a return statement");
    }
    eat_next(); // eat ';'

    return std::make_unique<return_stmt>(loc, std::move(e));
}

auto parser::parse_primary() -> std::unique_ptr<expr> {
    auto loc = next_token.loc;

    if (next_token.kind == token_kind::Number) {
        auto literal = std::make_unique<number_literal>(loc, *next_token.value);
        eat_next(); // eat literal
        return literal;
    }

    if (next_token.kind == token_kind::Identifier) {
        auto identifier = std::make_unique<decl_ref_expr>(loc, *next_token.value);
        eat_next(); // eat identifier
        return identifier;
    }

    if (next_token.kind == token_kind::Lpar) {
        eat_next(); // eat '('

        auto expr = parse_expr();
        if (!expr) {
            return nullptr;
        }

        if (next_token.kind != token_kind::Rpar) {
            return report(next_token.loc, "expected ')'");
        }
        eat_next(); // eat ')'

        return std::make_unique<grouping_expr>(loc, std::move(expr));
    }

    return report(next_token.loc, "expected expression");
}

auto parser::parse_prefix_expr() -> std::unique_ptr<expr> {
    auto tok = next_token;

    if (tok.kind != token_kind::Minus && tok.kind != token_kind::Excl) {
        return parse_postfix_expr();
    }

    eat_next(); // eat '-' or '!'

    auto rhs = parse_prefix_expr();
    if (!rhs) {
        return nullptr;
    }

    return std::make_unique<unary_op>(tok.loc, std::move(rhs), tok.kind);
}

auto parser::parse_postfix_expr() -> std::unique_ptr<expr> {
    auto expr = parse_primary();
    if (!expr) {
        return nullptr;
    }

    if (next_token.kind != token_kind::Lpar) {
        return expr;
    }

    auto loc = next_token.loc;
    auto arg_list = parse_arg_list();
    if (!arg_list) {
        return nullptr;
    }

    return std::make_unique<call_expr>(loc, std::move(expr), std::move(*arg_list));
}

auto parser::parse_param_decl() -> std::unique_ptr<param_decl> {
    auto loc = next_token.loc;

    std::string id = *next_token.value;
    eat_next(); // eat identifier

    if (next_token.kind != token_kind::Colon) {
        return report(next_token.loc, "expected ':'");
    }
    eat_next();

    auto type = parse_type();
    if (!type) {
        return nullptr;
    }

    return std::make_unique<param_decl>(loc, std::move(id), std::move(*type));
}
