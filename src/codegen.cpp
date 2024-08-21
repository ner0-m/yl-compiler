#include "codegen.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/ErrorHandling.h>

auto codegen::cur_fn() -> llvm::Function * { return builder.GetInsertBlock()->getParent(); };

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

    if (auto *if_stmt = dynamic_cast<const resolved_if_stmt *>(&stmt)) {
        return gen_if_stmt(*if_stmt);
    }

    if (auto *while_stmt = dynamic_cast<const resolved_while_stmt *>(&stmt)) {
        return gen_while_stmt(*while_stmt);
    }

    if (auto *decl_stmt = dynamic_cast<const resolved_decl_stmt *>(&stmt)) {
        return gen_decl_stmt(*decl_stmt);
    }

    llvm_unreachable("unknown statement");
}

auto codegen::gen_decl_stmt(const resolved_decl_stmt &stmt) -> llvm::Value * {
    auto *fn = cur_fn();

    const auto *decl = stmt.var.get();

    llvm::AllocaInst *var = allocate_stack_variable(fn, stmt.var->identifier);

    if (const auto &init = decl->initializer) {
        builder.CreateStore(gen_expr(*init), var);
    }

    declarations[decl] = var;
    return nullptr;
}

auto codegen::gen_if_stmt(const resolved_if_stmt &stmt) -> llvm::Value * {
    auto *function = cur_fn();

    auto *true_bb = llvm::BasicBlock::Create(context, "if.true", function);
    auto *exit_bb = llvm::BasicBlock::Create(context, "if.exit", function);

    auto *else_bb = exit_bb;
    if (stmt.false_block) {
        else_bb = llvm::BasicBlock::Create(context, "if.else", function);
    }

    auto *cond = gen_expr(*stmt.condition);
    builder.CreateCondBr(b2d(cond), true_bb, else_bb);

    true_bb->insertInto(function);
    builder.SetInsertPoint(true_bb);
    gen_block(*stmt.true_block);
    builder.CreateBr(exit_bb);

    if (stmt.false_block) {
        else_bb->insertInto(function);
        builder.SetInsertPoint(else_bb);
        gen_block(*stmt.false_block);
        builder.CreateBr(exit_bb);
    }

    exit_bb->insertInto(function);
    builder.SetInsertPoint(exit_bb);
    return nullptr;
}

auto codegen::gen_while_stmt(const resolved_while_stmt &stmt) -> llvm::Value * {
    auto *function = cur_fn();

    auto *header = llvm::BasicBlock::Create(context, "while.cond", function);
    auto *body = llvm::BasicBlock::Create(context, "while.body", function);
    auto *exit = llvm::BasicBlock::Create(context, "while.exit", function);

    builder.CreateBr(header);
    builder.SetInsertPoint(header);
    auto *cond = gen_expr(*stmt.condition);
    builder.CreateCondBr(d2b(cond), body, exit);

    builder.SetInsertPoint(body);
    gen_block(*stmt.body);
    builder.CreateBr(header);

    builder.SetInsertPoint(exit);
    return nullptr;
}

auto codegen::gen_expr(const resolved_expr &expr) -> llvm::Value * {
    if (auto val = expr.get_value()) {
        return llvm::ConstantFP::get(builder.getDoubleTy(), *val);
    }

    if (auto *number = dynamic_cast<const resolved_number_literal *>(&expr)) {
        return llvm::ConstantFP::get(builder.getDoubleTy(), number->value);
    }
    if (auto *dre = dynamic_cast<const resolved_decl_ref_expr *>(&expr)) {
        return builder.CreateLoad(builder.getDoubleTy(), declarations[dre->decl]);
    }
    if (auto *call = dynamic_cast<const resolved_call_expr *>(&expr)) {
        return gen_call_expr(*call);
    }
    if (auto *op = dynamic_cast<const resolved_unary_op *>(&expr)) {
        return gen_unary_op(*op);
    }
    if (auto *binop = dynamic_cast<const resolved_binary_op *>(&expr)) {
        return gen_binary_op(*binop);
    }
    if (auto *group = dynamic_cast<const resolved_grouping_expr *>(&expr)) {
        return gen_expr(*group->expr_);
    }
    llvm_unreachable("unexpected expression");
}

auto codegen::d2b(llvm::Value *val) -> llvm::Value * {
    return builder.CreateFCmpONE(val, llvm::ConstantFP::get(builder.getDoubleTy(), 0.0), "to.bool");
}

auto codegen::b2d(llvm::Value *val) -> llvm::Value * { return builder.CreateUIToFP(val, builder.getDoubleTy(), "to.double"); }

auto codegen::gen_unary_op(const resolved_unary_op &op) -> llvm::Value * {
    llvm::Value *res = gen_expr(*op.operand);

    if (op.op == token_kind::Minus) {
        return builder.CreateFNeg(res);
    }
    if (op.op == token_kind::Excl) {
        return b2d(builder.CreateNot(d2b(res)));
    }

    llvm_unreachable("unknown unary op");
    return nullptr;
}

auto codegen::gen_binary_op(const resolved_binary_op &op) -> llvm::Value * {
    auto kind = op.op;

    if (kind == token_kind::AmpAmp || kind == token_kind::PipePipe) {
        llvm::Function *function = cur_fn();
        bool isOr = kind == token_kind::PipePipe;

        auto *rhs_tag = isOr ? "or.rhs" : "and.rhs";
        auto *merge_tag = isOr ? "or.merge" : "and.merge";

        auto *rhs_block = llvm::BasicBlock::Create(context, rhs_tag, function);
        auto *merge_block = llvm::BasicBlock::Create(context, merge_tag, function);

        auto *true_block = isOr ? merge_block : rhs_block;
        auto *false_block = isOr ? rhs_block : merge_block;
        gen_conditional_op(*op.lhs, true_block, false_block);

        builder.SetInsertPoint(rhs_block);
        auto *rhs = d2b(gen_expr(*op.rhs));
        builder.CreateBr(merge_block);

        rhs_block = builder.GetInsertBlock();
        builder.SetInsertPoint(merge_block);
        llvm::PHINode *phi = builder.CreatePHI(builder.getInt1Ty(), 2);

        for (auto it = pred_begin(merge_block); it != pred_end(merge_block); ++it) {
            if (*it == rhs_block)
                phi->addIncoming(rhs, rhs_block);
            else
                phi->addIncoming(builder.getInt1(isOr), *it);
        }

        return b2d(phi);
    }

    llvm::Value *lhs = gen_expr(*op.rhs);
    llvm::Value *rhs = gen_expr(*op.lhs);

    if (kind == token_kind::Plus) {
        return builder.CreateFAdd(lhs, rhs);
    }
    if (kind == token_kind::Minus) {
        return builder.CreateFSub(lhs, rhs);
    }
    if (kind == token_kind::Asterisk) {
        return builder.CreateFMul(lhs, rhs);
    }
    if (kind == token_kind::Slash) {
        return builder.CreateFDiv(lhs, rhs);
    }
    if (kind == token_kind::Lt) {
        return b2d(builder.CreateFCmpOLT(lhs, rhs));
    }
    if (kind == token_kind::Gt) {
        return b2d(builder.CreateFCmpOGT(lhs, rhs));
    }
    if (kind == token_kind::EqualEqual) {
        return b2d(builder.CreateFCmpOEQ(lhs, rhs));
    }

    llvm_unreachable("unknown binary op");
    return nullptr;
}

auto codegen::gen_conditional_op(const resolved_expr &op, llvm::BasicBlock *true_block, llvm::BasicBlock *false_block) -> void {
    auto *function = cur_fn();

    const auto *binop = dynamic_cast<const resolved_binary_op *>(&op);

    if (binop && binop->op == token_kind::PipePipe) {
        llvm::BasicBlock *next_block = llvm::BasicBlock::Create(context, "or.lhs.false", function);
        gen_conditional_op(*binop->lhs, true_block, next_block);

        builder.SetInsertPoint(next_block);
        gen_conditional_op(*binop->rhs, true_block, false_block);

        return;
    }
    if (binop && binop->op == token_kind::AmpAmp) {
        llvm::BasicBlock *next_block = llvm::BasicBlock::Create(context, "or.lhs.true", function);
        gen_conditional_op(*binop->lhs, next_block, false_block);

        builder.SetInsertPoint(next_block);
        gen_conditional_op(*binop->rhs, true_block, false_block);

        return;
    }

    llvm::Value *val = d2b(gen_expr(op));
    builder.CreateCondBr(val, true_block, false_block);
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
