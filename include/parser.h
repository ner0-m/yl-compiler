#pragma once

#include <memory>
#include <optional>
#include <print>
#include <vector>

#include "ast.h"
#include "lexer.h"

auto tok_precedence(token_kind kind) -> i32;

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
    //   ::= <functionDecl>* EOF
    auto parse_source_file() -> std::pair<std::vector<std::unique_ptr<function_decl>>, bool>;

    // <functionDecl>
    //   ::= 'fn' <identifier> <parameterList> ':' <type> <block>
    auto parse_function_decl() -> std::unique_ptr<function_decl>;

    // <parameterList>
    //   ::= '(' (<paramDecl> (',' <paramDecl>)* ','?)? ')'
    auto parse_param_list() -> std::unique_ptr<std::vector<std::unique_ptr<param_decl>>>;

    // <paramDecl>
    //   ::= <identifier> ':' <type>
    auto parse_param_decl() -> std::unique_ptr<param_decl>;

    // <type>
    //   ::= 'number'
    //   |   'void'
    //   |   <identifier>
    auto parse_type() -> std::optional<type>;

    // <block>
    //   ::= '{' <statement>* '}'
    auto parse_block() -> std::unique_ptr<block>;

    // <statement>
    //  ::= <expr> ';'
    //  |   <returnStmt>
    //  |   <ifStatement>
    //  |   <whileStatement>
    //  |   <assignment>
    //  |   <declStmt>
    auto parse_stmt() -> std::unique_ptr<stmt>;

    // <declStmt>
    //  ::= ('let'|'var') <varDecl>  ';'
    auto parse_decl_stmt() -> std::unique_ptr<decl_stmt>;

    // <varDecl>
    //  ::= <identifier> (':' <type>)? ('=' <expr>)?
    auto parse_var_decl(bool is_let) -> std::unique_ptr<var_decl>;

    // <ifStatement>
    //   ::= 'if' <expr> <block> ('else' (<ifStatement> | <block>))?
    auto parse_if_stmt() -> std::unique_ptr<if_stmt>;

    // <whileStatement>
    //   ::= 'while' <expr> <block>
    auto parse_while_stmt() -> std::unique_ptr<while_stmt>;

    // <returnStmt>
    //   ::= 'return' <expr> ';'
    auto parse_return_stmt() -> std::unique_ptr<return_stmt>;

    // <expr>
    //   ::= <additiveExpression>
    //
    // <additiveExpression>
    //   ::= <multiplicativeExpression> (('+' | '-') <multiplicativeExpression>)*
    //
    // <multiplicativeExpression>
    //   ::= <primaryExpr> (('*' | '/') <primaryExpr>)*
    auto parse_expr() -> std::unique_ptr<expr>;

    // <prefixExpression>
    //   ::= ('!' | '-')* <postfixExpression>
    auto parse_prefix_expr() -> std::unique_ptr<expr>;

    // <postfixExpression>
    //   ::= <primaryExpression> <argumentList>

    // <argumentList>
    //   ::= '(' (<expr> (',' <expr>)* ','?)? ')'
    auto parse_postfix_expr() -> std::unique_ptr<expr>;

    auto parse_expr_rhs(std::unique_ptr<expr> lhs, i32 precedence) -> std::unique_ptr<expr>;

    // <primaryExpr>
    //   ::= <numberLiteral>
    //   |   <declRefExpr>
    //   |   '(' <expr> ')'
    //
    // <numberLiteral>
    //   ::= <number>
    //
    // <declRefExpr>
    //   ::= <identifier>
    //
    auto parse_primary() -> std::unique_ptr<expr>;

    // <argumentList>
    //   ::= '(' (<expr> (',' <expr>)* ','?)? ')'
    auto parse_arg_list() -> std::unique_ptr<std::vector<std::unique_ptr<expr>>>;
};
