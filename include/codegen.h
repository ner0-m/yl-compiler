#pragma once

#include <map>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/TargetParser/Host.h>

#include "ast.h"

class codegen {
    std::vector<std::unique_ptr<resolved_function_decl>> resolved_tree;

    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module module;

    llvm::Instruction *alloca_insert_point;

    std::map<const resolved_decl *, llvm::Value *> declarations;

    llvm::Value *ret_val = nullptr;
    llvm::BasicBlock *ret_bb = nullptr;

    auto gen_type(type t) -> llvm::Type *;

    auto gen_fn_decl(const resolved_function_decl &fn) -> void;

    auto gen_fn_body(const resolved_function_decl &fn) -> void;

    auto allocate_stack_variable(llvm::Function *function, const std::string_view identifier) -> llvm::AllocaInst *;

    auto gen_block(const resolved_block &block) -> void;

    auto gen_stmt(const resolved_stmt &stmt) -> llvm::Value *;

    auto gen_if_stmt(const resolved_if_stmt &stmt) -> llvm::Value *;

    auto gen_while_stmt(const resolved_while_stmt &stmt) -> llvm::Value *;

    auto gen_assignment(const resolved_assignment &stmt) -> llvm::Value *;

    auto gen_decl_stmt(const resolved_decl_stmt &stmt) -> llvm::Value *;

    auto gen_expr(const resolved_expr &stmt) -> llvm::Value *;

    auto gen_unary_op(const resolved_unary_op &op) -> llvm::Value *;

    auto gen_binary_op(const resolved_binary_op &op) -> llvm::Value *;

    auto gen_conditional_op(const resolved_expr &op, llvm::BasicBlock *true_block, llvm::BasicBlock *false_block) -> void;

    auto gen_return_stmt(const resolved_return_stmt &stmt) -> llvm::Value *;

    auto gen_call_expr(const resolved_call_expr &call) -> llvm::Value *;

    auto gen_builtin_print_body(const resolved_function_decl &println) -> void;

    auto gen_main_wrapper() -> void;

    auto d2b(llvm::Value *val) -> llvm::Value *;

    auto b2d(llvm::Value *val) -> llvm::Value *;

    auto cur_fn() -> llvm::Function *;

  public:
    codegen(std::vector<std::unique_ptr<resolved_function_decl>> tree, std::string_view source_path)
        : resolved_tree(std::move(tree)), context(), builder(context), module("<translation_unit>", context) {
        module.setSourceFileName(source_path);
        module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
    }

    auto generate_ir() -> llvm::Module *;

    auto dump() -> void { module.print(llvm::errs(), nullptr); }
};
