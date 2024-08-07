#pragma once

#include <memory>
#include <vector>

#include "utils.h"

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

    auto dump(usize level = 0) const -> void override;
};

struct number_literal : expr {
    std::string value;

    number_literal(source_location loc, std::string val) : expr(loc), value(std::move(val)) {}

    ~number_literal() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct decl_ref_expr : expr {
    std::string identifier;

    decl_ref_expr(source_location loc, std::string id) : expr(loc), identifier(std::move(id)) {}

    ~decl_ref_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct call_expr : expr {
    std::unique_ptr<expr> callee;
    std::vector<std::unique_ptr<expr>> arguments;

    call_expr(source_location loc, std::unique_ptr<expr> callee, std::vector<std::unique_ptr<expr>> arguments)
        : expr(loc), callee(std::move(callee)), arguments(std::move(arguments)) {}

    ~call_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct block {
    source_location loc;
    std::vector<std::unique_ptr<stmt>> statements;

    block(source_location pos, std::vector<std::unique_ptr<stmt>> stmts) : loc(pos), statements(std::move(stmts)) {}

    auto dump(usize level = 0) const -> void;
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

struct param_decl : decl {
    type type_;

    param_decl(source_location loc, std::string id, type t) : decl(loc, id), type_(t) {}

    ~param_decl() override = default;

    auto dump(usize level = 0) const -> void override;
};

struct function_decl : decl {
    type type_;
    std::unique_ptr<block> block_;
    std::vector<std::unique_ptr<param_decl>> params_;

    function_decl(source_location loc, std::string identifier, std::vector<std::unique_ptr<param_decl>> params, type t,
                  std::unique_ptr<block> b)
        : decl(loc, std::move(identifier)), params_(std::move(params)), type_(t), block_(std::move(b)) {}

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

struct resolved_expr : resolved_stmt {
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

    resolved_call_expr(source_location loc, resolved_function_decl &callee, std::vector<std::unique_ptr<resolved_expr>> args)
        : resolved_expr(loc, callee.type_), callee(&callee), arguments(std::move(args)) {}

    ~resolved_call_expr() override = default;

    auto dump(usize level = 0) const -> void override;
};
