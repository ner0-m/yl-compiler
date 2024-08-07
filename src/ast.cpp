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

auto param_decl::dump(u64 level) const -> void { std::print(std::cerr, "{}ParamDecl: {}:{}\n", indent(level), identifier, type_.name); }

auto function_decl::dump(u64 level) const -> void {
    std::print(std::cerr, "{}FunctionDecl: {}:{}\n", indent(level), identifier, type_.name);

    for (auto &&param : params_) {
        param->dump(level + 1);
    }

    block_->dump(level + 1);
}
