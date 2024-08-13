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

    auto gen_stmt(const resolved_stmt &stmt) -> llvm::Value*;

    auto gen_expr(const resolved_expr &stmt) -> llvm::Value *;

    auto gen_return_stmt(const resolved_return_stmt &stmt) -> llvm::Value *;

    auto gen_call_expr(const resolved_call_expr &call) -> llvm::Value *;

    auto gen_builtin_print_body(const resolved_function_decl& println) -> void;

    auto gen_main_wrapper() -> void;

  public:
    codegen(std::vector<std::unique_ptr<resolved_function_decl>> tree, std::string_view source_path)
        : resolved_tree(std::move(tree)), context(), builder(context), module("<translation unit>", context) {
        module.setSourceFileName(source_path);
        module.setTargetTriple(llvm::sys::getDefaultTargetTriple());
    }

    auto generate_ir() -> llvm::Module *;
};
