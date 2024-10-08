#pragma once

#include <memory>
#include <vector>

#include "lexer.h"
#include "utils.h"

template <class T> struct constant_value_container {
    std::optional<T> value = {};

  public:
    auto set_value(std::optional<T> v) -> void { value = v; }

    auto get_value() const -> std::optional<T> { return value; }
};

struct stmt {
    source_location loc;

    stmt(source_location pos) : loc(pos) {}

    virtual ~stmt() = default;

    virtual auto dump(usize level = 0) const -> void = 0;
};

struct expr : stmt {
    expr(source_location loc) : stmt(loc) {}

    ~expr() override = default;
};

struct return_stmt : stmt {
    std::unique_ptr<expr> expr_;

    return_stmt(source_location loc, std::unique_ptr<expr> e) : stmt(loc), expr_(std::move(e)) {}

    ~return_stmt() override = default;

    auto dump(usize level) const -> void override;
};

struct number_literal : expr {
    std::string value;

    number_literal(source_location loc, std::string val) : expr(loc), value(std::move(val)) {}

    ~number_literal() override = default;

    auto dump(usize level) const -> void override;
};

struct decl_ref_expr : expr {
    std::string identifier;

    decl_ref_expr(source_location loc, std::string id) : expr(loc), identifier(std::move(id)) {}

    ~decl_ref_expr() override = default;

    auto dump(usize level) const -> void override;
};

struct call_expr : expr {
    std::unique_ptr<expr> callee;
    std::vector<std::unique_ptr<expr>> arguments;

    call_expr(source_location loc, std::unique_ptr<expr> callee, std::vector<std::unique_ptr<expr>> arguments)
        : expr(loc), callee(std::move(callee)), arguments(std::move(arguments)) {}

    ~call_expr() override = default;

    auto dump(usize level) const -> void override;
};

struct unary_op : expr {
    std::unique_ptr<expr> operand;

    token_kind op;

    unary_op(source_location loc, std::unique_ptr<expr> operand, token_kind op) : expr(loc), operand(std::move(operand)), op(op) {}

    ~unary_op() override = default;

    auto dump(usize level) const -> void override;
};

struct binary_op : expr {
    std::unique_ptr<expr> lhs;
    std::unique_ptr<expr> rhs;

    token_kind op;

    binary_op(source_location loc, std::unique_ptr<expr> lhs, std::unique_ptr<expr> rhs, token_kind op)
        : expr(loc), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {}

    ~binary_op() override = default;

    auto dump(usize level) const -> void override;
};

struct grouping_expr : expr {
    std::unique_ptr<expr> expr_;

    grouping_expr(source_location loc, std::unique_ptr<expr> e) : expr(loc), expr_(std::move(e)) {}

    ~grouping_expr() override = default;

    auto dump(usize level) const -> void override;
};

struct block {
    source_location loc;
    std::vector<std::unique_ptr<stmt>> statements;

    block(source_location pos, std::vector<std::unique_ptr<stmt>> stmts) : loc(pos), statements(std::move(stmts)) {}

    auto dump(usize level = 0) const -> void;
};

struct assignment : stmt {
    std::unique_ptr<decl_ref_expr> variable;
    std::unique_ptr<expr> e;

    assignment(source_location loc, std::unique_ptr<decl_ref_expr> var, std::unique_ptr<expr> e)
        : stmt(loc), variable(std::move(var)), e(std::move(e)) {}

    ~assignment() override = default;

    auto dump(usize level) const -> void override;
};

struct if_stmt : stmt {
    std::unique_ptr<expr> condition;
    std::unique_ptr<block> true_block;
    std::unique_ptr<block> false_block;

    if_stmt(source_location loc, std::unique_ptr<expr> cond, std::unique_ptr<block> tblock, std::unique_ptr<block> fblock = nullptr)
        : stmt(loc), condition(std::move(cond)), true_block(std::move(tblock)), false_block(std::move(fblock)) {}

    ~if_stmt() override = default;

    auto dump(usize level) const -> void override;
};

struct while_stmt : stmt {
    std::unique_ptr<expr> condition;
    std::unique_ptr<block> body;

    while_stmt(source_location loc, std::unique_ptr<expr> cond, std::unique_ptr<block> body)
        : stmt(loc), condition(std::move(cond)), body(std::move(body)) {}

    ~while_stmt() override = default;

    auto dump(usize level) const -> void override;
};

struct type {
    enum class kind { void_, number, custom };

    kind k;
    std::string name;

    static auto builtin_void() -> type { return {kind::void_, "void"}; }

    static auto builtin_number() -> type { return {kind::number, "number"}; }

    static auto custom(std::string_view name) -> type { return {kind::custom, std::string(name)}; }

  private:
    type(kind k, std::string name) : k(k), name(std::move(name)){};
};

struct decl {
    source_location loc;
    std::string identifier;

    decl(source_location location, std::string id) : loc(location), identifier(id) {}

    virtual ~decl() = default;

    virtual auto dump(usize indent) const -> void = 0;
};

struct var_decl : decl {
    std::optional<type> type_;
    std::unique_ptr<expr> initializer;
    bool is_mutable;

    var_decl(source_location loc, std::string identifier, std::optional<type> t, bool is_mutable, std::unique_ptr<expr> init = nullptr)
        : decl(loc, std::move(identifier)), type_(t), is_mutable(is_mutable), initializer(std::move(init)) {}

    ~var_decl() override = default;

    auto dump(usize level) const -> void override;
};

struct decl_stmt : stmt {
    std::unique_ptr<var_decl> var;

    decl_stmt(source_location loc, std::unique_ptr<var_decl> var) : stmt(loc), var(std::move(var)) {}

    ~decl_stmt() override = default;

    auto dump(usize level) const -> void override;
};

struct param_decl : decl {
    type type_;

    param_decl(source_location loc, std::string id, type t) : decl(loc, id), type_(t) {}

    ~param_decl() override = default;

    auto dump(usize level) const -> void override;
};

struct function_decl : decl {
    type type_;
    std::unique_ptr<block> body_;
    std::vector<std::unique_ptr<param_decl>> params_;

    function_decl(source_location loc, std::string identifier, std::vector<std::unique_ptr<param_decl>> params, type t,
                  std::unique_ptr<block> b)
        : decl(loc, std::move(identifier)), params_(std::move(params)), type_(t), body_(std::move(b)) {}

    ~function_decl() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_decl {
    source_location loc;
    std::string identifier;
    type type_;

    resolved_decl(source_location loc, std::string id, type t) : loc(loc), identifier(id), type_(t) {}

    virtual ~resolved_decl() = default;

    virtual auto dump(usize level = 0) const -> void = 0;
};

struct resolved_param_decl : resolved_decl {
    resolved_param_decl(source_location loc, std::string id, type t) : resolved_decl(loc, id, t) {}

    ~resolved_param_decl() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_stmt {
    source_location loc;

    resolved_stmt(source_location loc) : loc(loc) {}

    virtual ~resolved_stmt() = default;

    virtual auto dump(usize level = 0) const -> void = 0;
};

struct resolved_block {
    source_location loc;
    std::vector<std::unique_ptr<resolved_stmt>> stmts;

    resolved_block(source_location loc, std::vector<std::unique_ptr<resolved_stmt>> stmts) : loc(loc), stmts(std::move(stmts)) {}

    auto dump(usize level = 0) const -> void;
};

struct resolved_function_decl : resolved_decl {
    std::vector<std::unique_ptr<resolved_param_decl>> params;
    std::unique_ptr<resolved_block> body;

    resolved_function_decl(source_location loc, std::string identifier, type t, std::vector<std::unique_ptr<resolved_param_decl>> params,
                           std::unique_ptr<resolved_block> body)
        : resolved_decl(loc, std::move(identifier), t), params(std::move(params)), body(std::move(body)) {}

    auto dump(usize level = 0) const -> void override;
};

struct resolved_expr : resolved_stmt, constant_value_container<double> {
    type t;

    resolved_expr(source_location loc, type t) : resolved_stmt(loc), t(t) {}

    virtual ~resolved_expr() = default;
};

struct resolved_return_stmt : public resolved_stmt {
    std::unique_ptr<resolved_expr> expr;

    resolved_return_stmt(source_location loc, std::unique_ptr<resolved_expr> expr = nullptr) : resolved_stmt(loc), expr(std::move(expr)) {}

    ~resolved_return_stmt() override = default;

    void dump(size_t level = 0) const override;
};

struct resolved_number_literal : resolved_expr {
    f64 value;

    resolved_number_literal(source_location loc, f64 val) : resolved_expr(loc, type::builtin_number()), value(val) {}

    ~resolved_number_literal() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_decl_ref_expr : resolved_expr {
    const resolved_decl *decl;

    resolved_decl_ref_expr(source_location loc, resolved_decl &decl) : resolved_expr(loc, decl.type_), decl(&decl) {}

    ~resolved_decl_ref_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_call_expr : resolved_expr {
    const resolved_function_decl *callee;
    std::vector<std::unique_ptr<resolved_expr>> arguments;

    resolved_call_expr(source_location loc, const resolved_function_decl &callee, std::vector<std::unique_ptr<resolved_expr>> args)
        : resolved_expr(loc, callee.type_), callee(&callee), arguments(std::move(args)) {}

    ~resolved_call_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_unary_op : resolved_expr {
    token_kind op;
    std::unique_ptr<resolved_expr> operand;

    resolved_unary_op(source_location loc, token_kind op, std::unique_ptr<resolved_expr> operand)
        : resolved_expr(loc, operand->t), op(op), operand(std::move(operand)) {}

    ~resolved_unary_op() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_binary_op : resolved_expr {
    token_kind op;
    std::unique_ptr<resolved_expr> lhs;
    std::unique_ptr<resolved_expr> rhs;

    resolved_binary_op(source_location loc, token_kind op, std::unique_ptr<resolved_expr> lhs, std::unique_ptr<resolved_expr> rhs)
        : resolved_expr(loc, lhs->t), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    ~resolved_binary_op() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_grouping_expr : resolved_expr {
    std::unique_ptr<resolved_expr> expr_;

    resolved_grouping_expr(source_location loc, std::unique_ptr<resolved_expr> e) : resolved_expr(loc, e->t), expr_(std::move(e)) {}

    ~resolved_grouping_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_if_stmt : resolved_stmt {
    std::unique_ptr<resolved_expr> condition;
    std::unique_ptr<resolved_block> true_block;
    std::unique_ptr<resolved_block> false_block;

    resolved_if_stmt(source_location loc, std::unique_ptr<resolved_expr> cond, std::unique_ptr<resolved_block> tblock,
                     std::unique_ptr<resolved_block> fblock = nullptr)
        : resolved_stmt(loc), condition(std::move(cond)), true_block(std::move(tblock)), false_block(std::move(fblock)) {}

    ~resolved_if_stmt() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_while_stmt : resolved_stmt {
    std::unique_ptr<resolved_expr> condition;
    std::unique_ptr<resolved_block> body;

    resolved_while_stmt(source_location loc, std::unique_ptr<resolved_expr> cond, std::unique_ptr<resolved_block> body)
        : resolved_stmt(loc), condition(std::move(cond)), body(std::move(body)) {}

    ~resolved_while_stmt() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct resolved_var_decl : resolved_decl {
    std::unique_ptr<resolved_expr> initializer;
    bool is_mutable;

    resolved_var_decl(source_location loc, std::string identifier, type t, bool is_mutable, std::unique_ptr<resolved_expr> init = nullptr)
        : resolved_decl(loc, std::move(identifier), t), is_mutable(is_mutable), initializer(std::move(init)) {}

    ~resolved_var_decl() override = default;

    auto dump(usize level) const -> void override;
};

struct resolved_decl_stmt : resolved_stmt {
    std::unique_ptr<resolved_var_decl> var;

    resolved_decl_stmt(source_location loc, std::unique_ptr<resolved_var_decl> var) : resolved_stmt(loc), var(std::move(var)) {}

    ~resolved_decl_stmt() override = default;

    auto dump(usize level) const -> void override;
};

struct resolved_assignment : resolved_stmt {
    std::unique_ptr<resolved_decl_ref_expr> variable;
    std::unique_ptr<resolved_expr> e;

    resolved_assignment(source_location loc, std::unique_ptr<resolved_decl_ref_expr> var, std::unique_ptr<resolved_expr> e)
        : resolved_stmt(loc), variable(std::move(var)), e(std::move(e)) {}

    ~resolved_assignment() override = default;

    auto dump(usize level) const -> void override;
};
