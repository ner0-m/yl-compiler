#include "sema.h"
#include "utils.h"

auto sema::resolve_ast() -> std::vector<std::unique_ptr<resolved_function_decl>> {
    std::vector<std::unique_ptr<resolved_function_decl>> resolved_tree;

    auto println = create_builtin_println();

    scope_raii global_scope(this);
    insert_to_cur_scope(*resolved_tree.emplace_back(std::move(println)));

    bool error = false;
    for (auto &&fn : ast) {
        auto resolved_fn_decl = resolve_function_decl(*fn);

        if (!resolved_fn_decl || !insert_to_cur_scope(*resolved_fn_decl)) {
            error = true;
            continue;
        }

        resolved_tree.emplace_back(std::move(resolved_fn_decl));
    }

    if (error) {
        return {};
    }

    for (usize i = 1; i < resolved_tree.size(); ++i) {
        cur_fn = resolved_tree[i].get();

        scope_raii param_scope(this);
        for (auto &&param : cur_fn->params) {
            insert_to_cur_scope(*param);
        }

        auto resolved_body = resolve_block(*ast[i - 1]->body_);
        if (!resolved_body) {
            error = true;
            continue;
        }

        cur_fn->body = std::move(resolved_body);
    }

    if (error) {
        return {};
    }

    return resolved_tree;
}

auto sema::resolve_type(type parsed_type) -> std::optional<type> {
    if (parsed_type.k == type::kind::custom) {
        return std::nullopt;
    }

    return parsed_type;
}

auto sema::resolve_function_decl(const function_decl &fn) -> std::unique_ptr<resolved_function_decl> {
    auto type = resolve_type(fn.type_);

    if (!type) {
        return report(fn.loc, std::format("function '{}' has invalid '{}' type", fn.identifier, fn.type_.name));
    }

    if (fn.identifier == "main") {
        if (type->k != type::kind::void_) {
            return report(fn.loc, "'main' function is expected to have 'void' type");
        }
        if (!fn.params_.empty()) {
            return report(fn.loc, "'main' function is expected to take no arguments");
        }
    }

    std::vector<std::unique_ptr<resolved_param_decl>> resolved_params;
    scope_raii param_scope(this);
    for (auto &&param : fn.params_) {
        auto resolved_param = resolve_param_decl(*param);

        if (!resolved_param || !insert_to_cur_scope(*resolved_param)) {
            return nullptr;
        }

        resolved_params.emplace_back(std::move(resolved_param));
    }

    return std::make_unique<resolved_function_decl>(fn.loc, fn.identifier, *type, std::move(resolved_params), nullptr);
}

auto sema::resolve_param_decl(const param_decl &param) -> std::unique_ptr<resolved_param_decl> {
    auto type = resolve_type(param.type_);

    if (!type || type->k == type::kind::void_) {
        return report(param.loc, std::format("parameter '{}' has invalid '{}' type", param.identifier, param.type_.name));
    }

    return std::make_unique<resolved_param_decl>(param.loc, param.identifier, *type);
}

auto sema::resolve_block(const block &block) -> std::unique_ptr<resolved_block> {
    std::vector<std::unique_ptr<resolved_stmt>> resolved_stmts;

    bool error = false;
    usize unreachable_count = 0;

    scope_raii block_scope(this);
    for (auto &&s : block.statements) {
        auto resolved_stmt = resolve_stmt(*s);

        error |= !resolved_stmts.emplace_back(std::move(resolved_stmt));
        if (error) {
            continue;
        }

        if (unreachable_count == 1) {
            report(s->loc, "unreachable statement", true);
            ++unreachable_count;
        }

        if (dynamic_cast<return_stmt *>(s.get())) {
            ++unreachable_count;
        }
    }

    if (error) {
        return nullptr;
    }

    return std::make_unique<resolved_block>(block.loc, std::move(resolved_stmts));
}

auto sema::resolve_stmt(const stmt &stmt) -> std::unique_ptr<resolved_stmt> {
    if (auto *expr = dynamic_cast<const struct expr *>(&stmt)) {
        return resolve_expr(*expr);
    }

    if (auto *return_stmt = dynamic_cast<const struct return_stmt *>(&stmt)) {
        return resolve_return_stmt(*return_stmt);
    }

    __builtin_unreachable();
}

auto sema::resolve_return_stmt(const return_stmt &ret_stmt) -> std::unique_ptr<resolved_return_stmt> {
    if (cur_fn->type_.k == type::kind::void_ && ret_stmt.expr_) {
        return report(ret_stmt.loc, "unexpected return value in void function");
    }

    if (cur_fn->type_.k != type::kind::void_ && !ret_stmt.expr_) {
        return report(ret_stmt.loc, "expected a return value");
    }

    std::unique_ptr<resolved_expr> resolved_expr;
    if (ret_stmt.expr_) {
        resolved_expr = resolve_expr(*ret_stmt.expr_);
        if (!resolved_expr) {
            return nullptr;
        }

        if (cur_fn->type_.k != resolved_expr->t.k) {
            return report(resolved_expr->loc, "unexpected return type");
        }
    }

    return std::make_unique<resolved_return_stmt>(ret_stmt.loc, std::move(resolved_expr));
}

auto sema::create_builtin_println() -> std::unique_ptr<resolved_function_decl> {
    source_location loc{"<builtin>", 0, 0};

    auto param = std::make_unique<resolved_param_decl>(loc, "n", type::builtin_number());

    std::vector<std::unique_ptr<resolved_param_decl>> params;
    params.emplace_back(std::move(param));

    auto block = std::make_unique<resolved_block>(loc, std::vector<std::unique_ptr<resolved_stmt>>{});

    return std::make_unique<resolved_function_decl>(loc, "println", type::builtin_void(), std::move(params), std::move(block));
}

auto sema::resolve_expr(const expr &expr) -> std::unique_ptr<resolved_expr> {
    if (const auto *number = dynamic_cast<const number_literal *>(&expr)) {
        return std::make_unique<resolved_number_literal>(number->loc, std::stod(number->value));
    }

    if (const auto *decl_ref_expr = dynamic_cast<const struct decl_ref_expr *>(&expr)) {
        return resolve_decl_ref_expr(*decl_ref_expr);
    }

    if (const auto *bop = dynamic_cast<const binary_op *>(&expr)) {
        return resolve_binary_op(*bop);
    }

    if (const auto *uop = dynamic_cast<const unary_op *>(&expr)) {
        return resolve_unary_op(*uop);
    }

    if (const auto *group = dynamic_cast<const grouping_expr *>(&expr)) {
        return resolve_group_expr(*group);
    }

    __builtin_unreachable();
}

auto sema::resolve_decl_ref_expr(const decl_ref_expr &decl_ref, bool is_callee) -> std::unique_ptr<resolved_decl_ref_expr> {
    resolved_decl *decl = lookupDecl(decl_ref.identifier).first;
    if (!decl) {
        return report(decl_ref.loc, std::format("symbol '{}' not found", decl_ref.identifier));
    }

    if (!is_callee && dynamic_cast<resolved_function_decl *>(decl)) {
        return report(decl_ref.loc, std::format("expected to call function '{}'", decl_ref.identifier));
    }

    return std::make_unique<resolved_decl_ref_expr>(decl_ref.loc, *decl);
}

auto sema::resolve_call_expr(const call_expr &call) -> std::unique_ptr<resolved_call_expr> {
    const auto *dre = dynamic_cast<const decl_ref_expr *>(call.callee.get());
    if (!dre) {
        return report(call.loc, "expression cannot be called as a function");
    }

    auto resolved_callee = resolve_decl_ref_expr(*dre, true);

    const auto *resolved_function_decl = dynamic_cast<const struct resolved_function_decl *>(resolved_callee->decl);

    if (!resolved_function_decl) {
        return report(call.loc, "calling non-function symbol");
    }

    if (call.arguments.size() != resolved_function_decl->params.size()) {
        return report(call.loc, "argument count mismatch in function call");
    }

    std::vector<std::unique_ptr<resolved_expr>> resolved_args;
    i64 idx = 0;
    for (auto &&arg : call.arguments) {
        auto resolved_arg = resolve_expr(*arg);

        if (resolved_arg->t.k != resolved_function_decl->params[idx]->type_.k)
            return report(resolved_arg->loc, "unexpected type of argument");

        ++idx;
        resolved_args.emplace_back(std::move(resolved_arg));
    }

    return std::make_unique<resolved_call_expr>(call.loc, *resolved_function_decl, std::move(resolved_args));
}

auto sema::resolve_unary_op(const unary_op &op) -> std::unique_ptr<resolved_unary_op> {
    auto resolved_rhs = resolve_expr(*op.operand);
    if (!resolved_rhs) {
        return nullptr;
    }

    if (resolved_rhs->t.k == type::kind::void_) {
        return report(resolved_rhs->loc, "void expression cannot be used as an operand to unary operator");
    }

    return std::make_unique<resolved_unary_op>(op.loc, op.op, std::move(resolved_rhs));
}

auto sema::resolve_binary_op(const binary_op &op) -> std::unique_ptr<resolved_binary_op> {
    auto resolved_lhs = resolve_expr(*op.lhs);
    if (!resolved_lhs) {
        return nullptr;
    }

    auto resolved_rhs = resolve_expr(*op.rhs);
    if (!resolved_rhs) {
        return nullptr;
    }

    if (resolved_lhs->t.k == type::kind::void_) {
        return report(resolved_lhs->loc, "void expression cannot be used as a lhs operand to binary operator");
    }

    if (resolved_rhs->t.k == type::kind::void_) {
        return report(resolved_rhs->loc, "void expression cannot be used as a rhs operand to binary operator");
    }

    return std::make_unique<resolved_binary_op>(op.loc, op.op, std::move(resolved_lhs), std::move(resolved_rhs));
}

auto sema::resolve_group_expr(const grouping_expr &group) -> std::unique_ptr<resolved_grouping_expr> {
    auto resolved_expr = resolve_expr(*group.expr_);
    if (!resolved_expr) {
        return nullptr;
    }
    return std::make_unique<resolved_grouping_expr>(group.loc, std::move(resolved_expr));
}

auto sema::lookupDecl(const std::string &id) const -> std::pair<resolved_decl *, int> {
    i64 scope_idx = 0;

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        for (auto &&decl : *it) {
            if (decl->identifier != id) {
                continue;
            }

            return {decl, scope_idx};
        }
        ++scope_idx;
    }

    return {nullptr, -1};
}

auto sema::insert_to_cur_scope(resolved_decl &decl) -> bool {
    const auto &[foundDecl, scopeIdx] = lookupDecl(decl.identifier);

    if (foundDecl && scopeIdx == 0) {
        report(decl.loc, "redeclaration of '" + decl.identifier + '\'');
        return false;
    }

    scopes.back().emplace_back(&decl);
    return true;
}
