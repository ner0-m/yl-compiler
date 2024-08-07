#pragma once

#include <memory>
#include <optional>
#include <print>
#include <vector>

#include "ast.h"
#include "lexer.h"

class parser {
    lexer *lex;
    token next_token;
    bool incomplete_ast = false;

  public:
    explicit parser(lexer &l) : lex(&l), next_token(lex->next_token()) {}

    auto eat_next() -> void;

    auto synchronize_on(token_kind kind) -> void;

    auto synchronize() -> void;

    // <sourceFile>
    //   ::= <function_decl>* EOF
    auto parse_source_file() -> std::pair<std::vector<std::unique_ptr<function_decl>>, bool>;

    // <functionDecl>
    //   ::= 'fn' <identifier> <parameterList> ':' <type> <block>
    auto parse_function_decl() -> std::unique_ptr<function_decl>;

    // <parameterList>
    //   ::= '(' (<paramDecl> (',' <paramDecl>)* ','?)? ')'
    auto parse_param_list() -> std::unique_ptr<std::vector<std::unique_ptr<param_decl>>>;

    auto parse_block() -> std::unique_ptr<block>;

    auto parse_arg_list() -> std::unique_ptr<std::vector<std::unique_ptr<expr>>>;

    // <type>
    //  ::= 'number'
    //  |   'void'
    //  |   <identifier>
    auto parse_type() -> std::optional<type>;

    // <statement>
    //   ::= <expr> ';'
    //   |   <returnStmt>
    auto parse_stmt() -> std::unique_ptr<stmt>;

    // <expr>
    //   ::= <postfixExpression>
    auto parse_expr() -> std::unique_ptr<expr>;

    // <returnStmt>
    //   ::= 'return' <expr> ';'
    auto parse_return_stmt() -> std::unique_ptr<return_stmt>;

    // <primaryExpr>
    //  ::= <numberLiteral>
    //  |   <declRefExpr>
    //  |   '(' <expr> ')'
    //
    // <numberLiteral>
    //  ::= <number>
    //
    // <declRefExpr>
    //  ::= <identifier>
    //
    auto parse_primary() -> std::unique_ptr<expr>;

    // <postfixExpression>
    //     ::= <primaryExpression> <argumentList>
    //
    // <argumentList>
    //     ::= '(' (<expr> (',' <expr>)* ','?)? ')'
    auto parse_postfix_expr() -> std::unique_ptr<expr>;

    // <paramDecl>
    //   ::= <identifier> ':' <type>
    auto parse_param_decl() -> std::unique_ptr<param_decl>;
};
