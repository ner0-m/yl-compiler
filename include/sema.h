#pragma once

#include <vector>

#include "ast.h"
#include "constexpr.h"

class sema {
    std::vector<std::unique_ptr<function_decl>> ast;

    std::vector<std::vector<resolved_decl *>> scopes;

    resolved_function_decl *cur_fn;

    constant_expression_evaluator cee;

    class scope_raii {
        sema *sema;

      public:
        scope_raii(class sema *s) : sema(s) { sema->scopes.emplace_back(); }

        ~scope_raii() { sema->scopes.pop_back(); }
    };

    auto lookupDecl(const std::string &id) const -> std::pair<resolved_decl *, int>;

    auto insert_to_cur_scope(resolved_decl &decl) -> bool;

    auto create_builtin_println() -> std::unique_ptr<resolved_function_decl>;

    auto resolve_type(type parsed_type) -> std::optional<type>;

    auto resolve_function_decl(const function_decl &fn) -> std::unique_ptr<resolved_function_decl>;

    auto resolve_param_decl(const param_decl &param) -> std::unique_ptr<resolved_param_decl>;

    auto resolve_block(const block &block) -> std::unique_ptr<resolved_block>;

    auto resolve_stmt(const stmt &stmt) -> std::unique_ptr<resolved_stmt>;

    auto resolve_return_stmt(const return_stmt &ret_stmt) -> std::unique_ptr<resolved_return_stmt>;

    auto resolve_expr(const expr &expr) -> std::unique_ptr<resolved_expr>;

    auto resolve_unary_op(const unary_op &expr) -> std::unique_ptr<resolved_unary_op>;

    auto resolve_binary_op(const binary_op &expr) -> std::unique_ptr<resolved_binary_op>;

    auto resolve_group_expr(const grouping_expr &expr) -> std::unique_ptr<resolved_grouping_expr>;

    auto resolve_decl_ref_expr(const decl_ref_expr &decl_ref, bool is_callee = false) -> std::unique_ptr<resolved_decl_ref_expr>;

    auto resolve_call_expr(const call_expr &call) -> std::unique_ptr<resolved_call_expr>;

  public:
    explicit sema(std::vector<std::unique_ptr<function_decl>> ast) : ast(std::move(ast)) {}

    auto resolve_ast() -> std::vector<std::unique_ptr<resolved_function_decl>>;
};
