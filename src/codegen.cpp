#include "codegen.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/ErrorHandling.h>

auto codegen::generate_ir() -> llvm::Module * {
    for (auto &&fn : resolved_tree) {
        gen_fn_decl(*fn);
    }

    for (auto &&fn : resolved_tree) {
        gen_fn_body(*fn);
    }

    gen_main_wrapper();

    return &module;
}

auto codegen::gen_main_wrapper() -> void {
    auto *builtin_main = module.getFunction("main");
    builtin_main->setName("__builtin_main");

    auto *main =
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {}, false), llvm::Function::ExternalLinkage, "main", module);

    auto *entry = llvm::BasicBlock::Create(context, "entry", main);
    builder.SetInsertPoint(entry);

    builder.CreateCall(builtin_main);
    builder.CreateRet(llvm::ConstantInt::getSigned(builder.getInt32Ty(), 0));
}

auto codegen::gen_type(type t) -> llvm::Type * {
    if (t.k == type::kind::number) {
        return builder.getDoubleTy();
    }

    return builder.getVoidTy();
}

auto codegen::gen_fn_decl(const resolved_function_decl &fn) -> void {
    auto *ret_type = gen_type(fn.type_);

    std::vector<llvm::Type *> param_types;
    for (auto &&param : fn.params) {
        param_types.emplace_back(gen_type(param->type_));
    }

    auto *type = llvm::FunctionType::get(ret_type, param_types, false);

    llvm::Function::Create(type, llvm::Function::ExternalLinkage, fn.identifier, module);
}

auto codegen::gen_fn_body(const resolved_function_decl &fn) -> void {
    auto *llvm_fn = module.getFunction(fn.identifier);

    auto *entryBB = llvm::BasicBlock::Create(context, "entry", llvm_fn);
    builder.SetInsertPoint(entryBB);

    llvm::Value *undef = llvm::UndefValue::get(builder.getInt32Ty());
    alloca_insert_point = new llvm::BitCastInst(undef, undef->getType(), "alloca.placeholder", entryBB);

    bool is_void = fn.type_.k == type::kind::void_;

    if (!is_void) {
        ret_val = allocate_stack_variable(llvm_fn, "retval");
    }

    ret_bb = llvm::BasicBlock::Create(context, "return");

    u32 idx = 0;
    for (auto &&arg : llvm_fn->args()) {
        const auto *param_decl = fn.params[idx].get();
        arg.setName(param_decl->identifier);

        llvm::Value *var = allocate_stack_variable(llvm_fn, fn.identifier);
        builder.CreateStore(&arg, var);

        declarations[param_decl] = var;
        ++idx;
    }

    if (fn.identifier == "println") {
        gen_builtin_print_body(fn);
    } else {
        gen_block(*fn.body);
    }

    if (ret_bb->hasNPredecessorsOrMore(1)) {
        builder.CreateBr(ret_bb);
        ret_bb->insertInto(llvm_fn);
        builder.SetInsertPoint(ret_bb);
    }

    alloca_insert_point->eraseFromParent();
    alloca_insert_point = nullptr;

    if (is_void) {
        builder.CreateRetVoid();
    } else {
        builder.CreateRet(builder.CreateLoad(builder.getDoubleTy(), ret_val));
    }
}

auto codegen::allocate_stack_variable(llvm::Function *function, const std::string_view identifier) -> llvm::AllocaInst * {
    llvm::IRBuilder<> tmpBuilder(context);
    tmpBuilder.SetInsertPoint(alloca_insert_point);

    return tmpBuilder.CreateAlloca(tmpBuilder.getDoubleTy(), nullptr, identifier);
}

auto codegen::gen_block(const resolved_block &block) -> void {
    for (auto &&stmt : block.stmts) {
        gen_stmt(*stmt);

        if (dynamic_cast<const resolved_return_stmt *>(stmt.get())) {
            builder.ClearInsertionPoint();
            break;
        }
    }
}

auto codegen::gen_stmt(const resolved_stmt &stmt) -> llvm::Value * {
    if (auto *expr = dynamic_cast<const resolved_expr *>(&stmt)) {
        return gen_expr(*expr);
    }

    if (auto *ret_stmt = dynamic_cast<const resolved_return_stmt *>(&stmt)) {
        return gen_return_stmt(*ret_stmt);
    }

    llvm_unreachable("unknown statement");
}

auto codegen::gen_expr(const resolved_expr &expr) -> llvm::Value * {
    if (auto *number = dynamic_cast<const resolved_number_literal *>(&expr)) {
        return llvm::ConstantFP::get(builder.getDoubleTy(), number->value);
    }
    if (auto *dre = dynamic_cast<const resolved_decl_ref_expr *>(&expr)) {
        return builder.CreateLoad(builder.getDoubleTy(), declarations[dre->decl]);
    }
    if (auto *call = dynamic_cast<const resolved_call_expr *>(&expr)) {
        return gen_call_expr(*call);
    }
    llvm_unreachable("unexpected expression");
}

auto codegen::gen_call_expr(const resolved_call_expr &call) -> llvm::Value * {
    auto *callee = module.getFunction(call.callee->identifier);

    std::vector<llvm::Value *> args;
    for (auto &&arg : call.arguments) {
        args.emplace_back(gen_expr(*arg));
    }

    return builder.CreateCall(callee, args);
}

auto codegen::gen_return_stmt(const resolved_return_stmt &stmt) -> llvm::Value * {
    if (stmt.expr) {
        builder.CreateStore(gen_expr(*stmt.expr), ret_val);
    }

    return builder.CreateBr(ret_bb);
}

auto codegen::gen_builtin_print_body(const resolved_function_decl &println) -> void {
    auto *type = llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, true);

    auto *printf = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "printf", module);
    auto *fmt = builder.CreateGlobalStringPtr("%.15g\n");
    llvm::Value *param = builder.CreateLoad(builder.getDoubleTy(), declarations[println.params[0].get()]);
    builder.CreateCall(printf, {fmt, param});
}
