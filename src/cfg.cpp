#include "cfg.h"

auto cfg_builder::is_terminator(const resolved_stmt &stmt) -> bool {
    return dynamic_cast<const resolved_if_stmt *>(&stmt) || dynamic_cast<const resolved_while_stmt *>(&stmt) ||
           dynamic_cast<const resolved_return_stmt *>(&stmt);
}

auto cfg_builder::insert_block(const resolved_block &block, usize succ) -> usize {
    const auto &stmts = block.stmts;

    auto insert_new_block = true;
    for (auto it = stmts.rbegin(); it != stmts.rend(); ++it) {
        if (insert_new_block && !is_terminator(**it)) {
            succ = graph.insert_before(succ, true);
        }

        insert_new_block = dynamic_cast<const resolved_while_stmt *>(it->get());
        succ = insert_stmt(**it, succ);
    }

    return succ;
}

auto cfg_builder::insert_stmt(const resolved_stmt &stmt, usize block) -> usize {
    if (auto *if_stmt = dynamic_cast<const resolved_if_stmt *>(&stmt)) {
        return insert_if_stmt(*if_stmt, block);
    }

    if (auto *while_stmt = dynamic_cast<const resolved_while_stmt *>(&stmt)) {
        return insert_while_stmt(*while_stmt, block);
    }

    if (auto *expr = dynamic_cast<const resolved_expr *>(&stmt)) {
        return insert_expr(*expr, block);
    }

    if (auto *return_stmt = dynamic_cast<const resolved_return_stmt *>(&stmt)) {
        return insert_return_stmt(*return_stmt, block);
    }

    if (auto *decl_stmt = dynamic_cast<const resolved_decl_stmt *>(&stmt)) {
        return insert_decl_stmt(*decl_stmt, block);
    }

    llvm_unreachable("unexpected expression");
}

auto cfg_builder::insert_if_stmt(const resolved_if_stmt &stmt, usize exit) -> usize {
    auto false_block = exit;

    if (stmt.false_block) {
        false_block = insert_block(*stmt.false_block, exit);
    }

    auto true_block = insert_block(*stmt.true_block, exit);
    auto entry = graph.insert_end();

    auto val = cee.evaluate(*stmt.condition, true);
    graph.insert_edge(entry, true_block, val.value_or(1) != 0);
    graph.insert_edge(entry, false_block, val.value_or(0) == 0);

    graph.insert_stmt(&stmt, entry);
    return insert_expr(*stmt.condition, entry);
}

auto cfg_builder::insert_while_stmt(const resolved_while_stmt &stmt, usize exit) -> usize {
    auto latch = graph.insert_end();
    auto body = insert_block(*stmt.body, latch);

    auto header = graph.insert_end();
    graph.insert_edge(latch, header, true);

    auto val = cee.evaluate(*stmt.condition, true);
    graph.insert_edge(header, body, val != 0);
    graph.insert_edge(header, exit, val.value_or(0) == 0);

    graph.insert_stmt(&stmt, header);
    insert_expr(*stmt.condition, header);

    return header;
}

auto cfg_builder::insert_expr(const resolved_expr &expr, usize block) -> usize {
    graph.insert_stmt(&expr, block);

    if (auto *call = dynamic_cast<const resolved_call_expr *>(&expr)) {
        for (auto it = call->arguments.rbegin(); it != call->arguments.rend(); ++it) {
            insert_expr(**it, block);
        }
        return block;
    }

    if (auto *grouping = dynamic_cast<const resolved_grouping_expr *>(&expr)) {
        return insert_expr(*grouping->expr_, block);
    }

    if (auto *binop = dynamic_cast<const resolved_binary_op *>(&expr)) {
        return insert_expr(*binop->rhs, block), insert_expr(*binop->lhs, block);
    }

    if (auto *uop = dynamic_cast<const resolved_unary_op *>(&expr)) {
        return insert_expr(*uop->operand, block);
    }

    return block;
}

auto cfg_builder::insert_return_stmt(const resolved_return_stmt &stmt, usize block) -> usize {
    block = graph.insert_before(graph.exit, true);

    graph.insert_stmt(&stmt, block);
    if (stmt.expr) {
        return insert_expr(*stmt.expr, block);
    }

    return block;
}

auto cfg_builder::insert_decl_stmt(const resolved_decl_stmt &stmt, usize block) -> usize {
    graph.insert_stmt(&stmt, block);

    if (const auto &init = stmt.var->initializer) {
        return insert_expr(*stmt.var->initializer, block);
    }

    return block;
}

auto cfg_builder::build(const resolved_function_decl &fn) -> cfg {
    graph = {};
    graph.exit = graph.insert_end();

    auto body = insert_block(*fn.body, graph.exit);

    graph.entry = graph.insert_before(body, true);
    return graph;
}
