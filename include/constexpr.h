#pragma once

#include <optional>

#include "ast.h"

class constant_expression_evaluator {

    auto eval_unary_op(const resolved_unary_op &uop, bool allow_side_effects) const -> std::optional<double>;

    auto eval_binary_op(const resolved_binary_op &bop, bool allow_side_effects) const -> std::optional<double>;

    auto eval_decl_ref_expr(const resolved_decl_ref_expr &bop, bool allow_side_effects) const -> std::optional<double>;
  public:
    auto evaluate(const resolved_expr &expr, bool allow_side_effects) const -> std::optional<double>;
};
