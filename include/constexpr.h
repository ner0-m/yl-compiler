#pragma once

#include <optional>

#include "ast.h"

class constant_expression_evaluator {
  public:
    auto evaluate(const resolved_expr &expr, bool allow_side_effects) const -> std::optional<double>;

    auto eval_unary_op(const resolved_unary_op &uop, bool allow_side_effects) const -> std::optional<double>;

    auto eval_binary_op(const resolved_binary_op &bop, bool allow_side_effects) const -> std::optional<double>;
};
