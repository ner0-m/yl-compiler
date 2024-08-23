#pragma once

#include <print>
#include <set>
#include <vector>

#include <llvm/Support/ErrorHandling.h>

#include "ast.h"
#include "constexpr.h"

struct basic_block {
    std::set<std::pair<int, bool>> predecessor;
    std::set<std::pair<int, bool>> successor;
    std::vector<const resolved_stmt *> stmts;
};

struct cfg {
    std::vector<basic_block> basic_blocks;
    i32 entry = -1;
    i32 exit = -1;

    auto insert_end() -> usize {
        basic_blocks.emplace_back();
        return basic_blocks.size() - 1;
    }

    auto insert_before(usize before, bool reachable) -> usize {
        auto b = insert_end();
        insert_edge(b, before, reachable);
        return b;
    }

    auto insert_edge(usize from, usize to, bool reachable) -> void {
        basic_blocks[from].successor.emplace(std::make_pair(to, reachable));
        basic_blocks[to].predecessor.emplace(std::make_pair(from, reachable));
    }

    auto insert_stmt(const resolved_stmt *stmt, usize block) { basic_blocks[block].stmts.emplace_back(stmt); }

    auto dump() const {
        i32 i = basic_blocks.size() - 1;
        for (auto it = basic_blocks.rbegin(); it != basic_blocks.rend(); --i, ++it) {
            auto txt = [&]() {
                if (i == entry)
                    return " (entry)";
                else if (i == exit)
                    return " (exit)";
                return "";
            }();

            std::print("[{}{}]\n", i, txt);
            std::print("    preds: ");
            for (auto &&[id, reachable] : it->predecessor) {
                std::print("{}{}", id, (reachable) ? " " : "(U) ");
            }
            std::print("\n");

            std::print("    succs: ");
            for (auto &&[id, reachable] : it->successor) {
                std::print("{}{}", id, (reachable) ? " " : "(U) ");
            }
            std::print("\n");

            const auto &stmts = it->stmts;
            for (auto it = stmts.rbegin(); it != stmts.rend(); ++it) {
                (*it)->dump(1);
            }
            std::print("\n");
        }
    }
};

class cfg_builder {
    constant_expression_evaluator cee;
    cfg graph;

    auto is_terminator(const resolved_stmt &stmt) -> bool;

    auto insert_block(const resolved_block &block, usize succ) -> usize;

    auto insert_stmt(const resolved_stmt &stmt, usize block) -> usize;

    auto insert_if_stmt(const resolved_if_stmt &stmt, usize exit) -> usize;

    auto insert_while_stmt(const resolved_while_stmt &stmt, usize exit) -> usize;

    auto insert_expr(const resolved_expr &expr, usize block) -> usize;

    auto insert_return_stmt(const resolved_return_stmt &stmt, usize block) -> usize;

    auto insert_decl_stmt(const resolved_decl_stmt &stmt, usize block) -> usize;

    auto insert_assignment(const resolved_assignment &stmt, usize block) -> usize;

  public:
    auto build(const resolved_function_decl &fn) -> cfg;
};
