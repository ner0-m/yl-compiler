#pragma once

#include <vector>

#include "ast.h"
#include "cfg.h"
#include "constexpr.h"

class sema {
    std::vector<std::unique_ptr<function_decl>> ast;

    std::vector<std::vector<resolved_decl *>> scopes;

    resolved_function_decl *cur_fn;

    constant_expression_evaluator cee;

    class scope_raii {
        sema *sema_;

      public:
        scope_raii(sema *s) : sema_(s) { sema_->scopes.emplace_back(); }

        ~scope_raii() { sema_->scopes.pop_back(); }
    };

    auto check_return_on_all_paths(const resolved_function_decl &fn, const cfg &graph) const -> bool;

    auto flow_sensitive_checks(const resolved_function_decl &fn) const -> bool;

    auto check_variable_initialization(const resolved_function_decl &decl, const cfg &graph) const -> bool;

    auto lookupDecl(const std::string &id) const -> std::pair<resolved_decl *, int>;

    auto insert_to_cur_scope(resolved_decl &decl) -> bool;

    auto create_builtin_println() -> std::unique_ptr<resolved_function_decl>;

    auto resolve_type(type parsed_type) -> std::optional<type>;

    auto resolve_function_decl(const function_decl &fn) -> std::unique_ptr<resolved_function_decl>;

    auto resolve_param_decl(const param_decl &param) -> std::unique_ptr<resolved_param_decl>;

    auto resolve_block(const block &block) -> std::unique_ptr<resolved_block>;

    auto resolve_stmt(const stmt &stmt) -> std::unique_ptr<resolved_stmt>;

    auto resolve_assignment(const assignment &stmt) -> std::unique_ptr<resolved_assignment>;

    auto resolve_decl_stmt(const decl_stmt &stmt) -> std::unique_ptr<resolved_decl_stmt>;

    auto resolve_var_decl(const var_decl &stmt) -> std::unique_ptr<resolved_var_decl>;

    auto resolve_if_stmt(const if_stmt &stmt) -> std::unique_ptr<resolved_if_stmt>;

    auto resolve_while_stmt(const while_stmt &stmt) -> std::unique_ptr<resolved_while_stmt>;

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
