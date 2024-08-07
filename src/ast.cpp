#include <iostream>

#include "ast.h"

auto return_stmt::dump(usize level) const -> void {
    std::cerr << std::format("{}ReturnStmt\n", indent(level));

    if (expr_) {
        expr_->dump(level + 1);
    }
}

auto number_literal::dump(usize level) const -> void { std::cerr << std::format("{}NumberLiteral: '{}'\n", indent(level), value); }

auto decl_ref_expr::dump(usize level) const -> void { std::cerr << std::format("{}DeclRefExpr: '{}'\n", indent(level), identifier); }

auto call_expr::dump(usize level) const -> void {
    std::print(std::cerr, "{}CallExpr:\n", indent(level));

    callee->dump(level + 1);
    for (auto &&arg : arguments) {
        arg->dump(level + 1);
    }
}

auto block::dump(usize level) const -> void {
    std::cerr << std::format("{}Block\n", indent(level));

    for (auto &&s : statements) {
        s->dump(level + 1);
    }
};

auto param_decl::dump(usize level) const -> void { std::print(std::cerr, "{}ParamDecl: {}:{}\n", indent(level), identifier, type_.name); }

auto function_decl::dump(usize level) const -> void {
    std::print(std::cerr, "{}FunctionDecl: {}:{}\n", indent(level), identifier, type_.name);

    for (auto &&param : params_) {
        param->dump(level + 1);
    }

    block_->dump(level + 1);
}

void resolved_param_decl::dump(usize level) const {
    std::print(std::cerr, "{}ResolvedParamDecl: @({}) {}:\n", indent(level), static_cast<const void *>(this), identifier);
}

auto resolved_function_decl::dump(usize level) const -> void {
    std::print(std::cerr, "{}ResolvedFunctionDecl: @({}) {}:\n", indent(level), static_cast<const void *>(this), identifier);
    for (auto &&param : params)
        param->dump(level + 1);

    body->dump(level + 1);
}

void resolved_number_literal::dump(usize level) const { std::print(std::cerr, "{}ResolvedNumberLiteral: '{}'\n", indent(level), value); }

void resolved_decl_ref_expr::dump(usize level) const {
    std::print(std::cerr, "{}ResolvedDeclRefExpr: @({}) {}\n", indent(level), static_cast<const void *>(decl), decl->identifier);
}

void resolved_call_expr::dump(usize level) const {
    std::print(std::cerr, "{}ResolvedCallExpr: @({}) {}\n", indent(level), static_cast<const void *>(callee), callee->identifier);
    for (auto &&arg : arguments) {
        arg->dump(level + 1);
    }
}

auto resolved_block::dump(usize level) const -> void {
    std::print(std::cerr, "{}ResolvedBlock: \n", indent(level));

    for (auto &&stmt : stmts) {
        stmt->dump(level + 1);
    }
}

auto resolved_return_stmt::dump(usize level) const -> void {
    std::print(std::cerr, "{}ResolvedReturnStmt: \n", indent(level));

    if (expr) {
        expr->dump(level + 1);
    }
}
