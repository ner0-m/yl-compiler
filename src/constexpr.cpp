#include "constexpr.h"
#include "ast.h"

#include <llvm/Support/ErrorHandling.h>

auto constant_expression_evaluator::evaluate(const resolved_expr &expr, bool allow_side_effects) const -> std::optional<double> {
    if (auto val = expr.get_value()) {
        return val;
    }

    if (auto *num = dynamic_cast<const resolved_number_literal *>(&expr)) {
        return num->value;
    }

    if (auto *group = dynamic_cast<const resolved_grouping_expr *>(&expr)) {
        return evaluate(*group->expr_, allow_side_effects);
    }

    if (auto *uop = dynamic_cast<const resolved_unary_op *>(&expr)) {
        return eval_unary_op(*uop, allow_side_effects);
    }

    if (auto *bop = dynamic_cast<const resolved_binary_op *>(&expr)) {
        return eval_binary_op(*bop, allow_side_effects);
    }

    if (auto *decl_ref_expr = dynamic_cast<const resolved_decl_ref_expr *>(&expr)) {
        return eval_decl_ref_expr(*decl_ref_expr, allow_side_effects);
    }

    return {};
}

auto constant_expression_evaluator::eval_decl_ref_expr(const resolved_decl_ref_expr &dre,
                                                       bool allow_side_effects) const -> std::optional<double> {
    const auto *rvd = dynamic_cast<const resolved_var_decl *>(dre.decl);
    if (!rvd || rvd->is_mutable || !rvd->initializer) {
        return std::nullopt;
    }

    return evaluate(*rvd->initializer, allow_side_effects);
}

auto constant_expression_evaluator::eval_unary_op(const resolved_unary_op &uop, bool allow_side_effects) const -> std::optional<double> {
    auto operand = evaluate(*uop.operand, allow_side_effects);

    if (uop.op == token_kind::Excl) {
        return operand.transform([](auto x) { return !(x != 0.0); });
    }

    if (uop.op == token_kind::Minus) {
        return operand.transform([](auto x) { return -x; });
    }

    llvm_unreachable("message");

    return {};
}

auto constant_expression_evaluator::eval_binary_op(const resolved_binary_op &bop, bool allow_side_effects) const -> std::optional<double> {
    auto lhs = evaluate(*bop.lhs, allow_side_effects);
    if (!lhs && !allow_side_effects) {
        return std::nullopt;
    }

    if (bop.op == token_kind::PipePipe) {
        if ((lhs.value() != 0.0) == true) {
            return 1.0;
        }

        auto rhs = evaluate(*bop.rhs, allow_side_effects);
        if (rhs.has_value() && (rhs != 0.0) == true) {
            return 1.0;
        }

        if (lhs.has_value() && rhs.has_value()) {
            return 0.0;
        }

        return {};
    }

    if (bop.op == token_kind::AmpAmp) {
        if ((lhs.value() != 0.0) == false) {
            return 0.0;
        }

        auto rhs = evaluate(*bop.rhs, allow_side_effects);
        if (rhs.has_value() && (rhs != 0.0) == false) {
            return 0.0;
        }

        if (lhs.has_value() && rhs.has_value()) {
            return 1.0;
        }

        return {};
    }

    if (!lhs.has_value()) {
        return {};
    }

    auto rhs = evaluate(*bop.rhs, allow_side_effects);
    if (!rhs.has_value()) {
        return {};
    }

    switch (bop.op) {
    case token_kind::Asterisk:
        return lhs.value() * rhs.value();
    case token_kind::Slash:
        return lhs.value() / rhs.value();
    case token_kind::Plus:
        return lhs.value() + rhs.value();
    case token_kind::Minus:
        return lhs.value() - rhs.value();
    case token_kind::Lt:
        return lhs.value() < rhs.value();
    case token_kind::Gt:
        return lhs.value() > rhs.value();
    case token_kind::EqualEqual:
        return lhs.value() == rhs.value();
    default:
        llvm_unreachable("unexpected binary operator");
    }

    return {};
}
