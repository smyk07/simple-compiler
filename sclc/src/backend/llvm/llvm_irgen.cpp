/*
 * llvm_irgen: generates LLVM IR from the passed scull AST.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "sclc/backend/llvm/llvm_irgen.hpp"

extern "C" {
#include "frontend/ast.h"
#include "frontend/var.h"

#include "core/common.h"
#include "core/ds/dynamic_array.h"
#include "core/utils.h"
}

#include "llvm/Support/ErrorHandling.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>

#include <cinttypes>
#include <map>

static llvm::Type *scl_type_to_llvm(llvm_backend_ctx &ctx, type t) {
  switch (t) {
  case TYPE_U8:
  case TYPE_I8:
    return llvm::Type::getInt8Ty(*ctx.context);
  case TYPE_U16:
  case TYPE_I16:
    return llvm::Type::getInt16Ty(*ctx.context);
  case TYPE_U32:
  case TYPE_I32:
    return llvm::Type::getInt32Ty(*ctx.context);
  case TYPE_U64:
  case TYPE_I64:
    return llvm::Type::getInt64Ty(*ctx.context);
  case TYPE_U128:
  case TYPE_I128:
    return llvm::Type::getInt128Ty(*ctx.context);

  case TYPE_BOOL:
    return llvm::Type::getInt1Ty(*ctx.context);

  case TYPE_CHAR:
    return llvm::Type::getInt8Ty(*ctx.context);

  case TYPE_POINTER:
    return llvm::PointerType::get(*ctx.context, 0);

  case TYPE_STRING:
    return llvm::PointerType::get(*ctx.context, 0);

  case TYPE_INVALID:
  case TYPE_VOID:
    return llvm::Type::getVoidTy(*ctx.context);
  }
}

static llvm::Value *cast_to_type(llvm_backend_ctx &ctx, llvm::Value *val,
                                 llvm::Type *target_type) {
  if (val->getType() == target_type)
    return val;
  return ctx.builder->CreateIntCast(val, target_type, true, "cast");
}

static std::map<std::string, llvm::AllocaInst *> named_values;

void llvm_irgen_clear_symbol_table() { named_values.clear(); }

static std::map<std::string, llvm::BasicBlock *> label_blocks;

static llvm::AllocaInst *create_entry_block_alloca(llvm::Function *fn,
                                                   const std::string &var_name,
                                                   llvm::Type *type) {

  llvm::IRBuilder<> tmp_builder(&fn->getEntryBlock(),
                                fn->getEntryBlock().begin());

  return tmp_builder.CreateAlloca(type, nullptr, var_name);
}

static llvm::Value *llvm_irgen_arithmetic_expr(llvm_backend_ctx &ctx,
                                               arithmetic_expr_node *expr);

static llvm::Value *llvm_irgen_term(llvm_backend_ctx &ctx, term_node *term) {
  switch (term->kind) {
  case TERM_INT:
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx.context),
                                  term->value.integer, true);

  case TERM_CHAR:
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*ctx.context),
                                  term->value.character, false);

  case TERM_STRING: {
    llvm::Constant *str_const =
        llvm::ConstantDataArray::getString(*ctx.context, term->value.str, true);

    llvm::GlobalVariable *global_str = new llvm::GlobalVariable(
        *ctx.module, str_const->getType(), true,
        llvm::GlobalValue::PrivateLinkage, str_const, ".str");

    return llvm::ConstantExpr::getBitCast(
        global_str, llvm::PointerType::get(*ctx.context, 0));
  }

  case TERM_IDENTIFIER: {
    auto it = named_values.find(term->identifier.name);
    if (it == named_values.end()) {
      scu_perror(
          const_cast<char *>("Unknown variable '%s' at line %" PRIu64 "\n"),
          term->identifier.name, term->line);
      return nullptr;
    }

    llvm::AllocaInst *alloca = it->second;
    llvm::Type *alloca_type = alloca->getAllocatedType();

    if (alloca_type->isArrayTy()) {
      return alloca;
    }

    llvm::Type *type = alloca->getAllocatedType();

    if (type->isArrayTy())
      return alloca;

    return ctx.builder->CreateLoad(type, alloca, term->identifier.name);
  }

  case TERM_FUNCTION_CALL: {
    fn_call_node *call = &term->fn_call;

    llvm::Function *callee = ctx.module->getFunction(call->name);
    if (!callee) {
      scu_perror(
          const_cast<char *>("Unknown function '%s' at line %" PRIu64 "\n"),
          call->name, term->line);
      return nullptr;
    }

    std::vector<llvm::Value *> args;
    for (u64 i = 0; i < call->parameters.count; i++) {
      arithmetic_expr_node arg;
      dynamic_array_get(&call->parameters, i, &arg);

      llvm::Value *arg_val = llvm_irgen_arithmetic_expr(ctx, &arg);
      if (!arg_val)
        return nullptr;
      args.push_back(arg_val);
    }

    return ctx.builder->CreateCall(callee, args, "calltmp");
  }

  case TERM_DEREF: {
    auto it = named_values.find(term->identifier.name);
    if (it == named_values.end()) {
      scu_perror(const_cast<char *>(
                     "Unknown pointer variable '%s' at line %" PRIu64 "\n"),
                 term->identifier.name, term->line);
      return nullptr;
    }

    llvm::AllocaInst *ptr_alloca = it->second;

    llvm::Value *ptr = ctx.builder->CreateLoad(ptr_alloca->getAllocatedType(),
                                               ptr_alloca, "ptr");

    llvm::Type *pointee_type = llvm::Type::getInt8Ty(*ctx.context);
    return ctx.builder->CreateLoad(pointee_type, ptr, "deref");
  }

  case TERM_ADDOF: {
    auto it = named_values.find(term->identifier.name);
    if (it == named_values.end()) {
      scu_perror(
          const_cast<char *>("Unknown variable '%s' at line %" PRIu64 "\n"),
          term->identifier.name, term->line);
      return nullptr;
    }

    return it->second;
  }

  case TERM_ARRAY_ACCESS: {
    array_access_node *access = &term->array_access;

    auto it = named_values.find(access->array_var.name);
    if (it == named_values.end()) {
      scu_perror(const_cast<char *>("Unknown array '%s' at line %" PRIu64 "\n"),
                 access->array_var.name, term->line);
      return nullptr;
    }

    llvm::AllocaInst *array_alloca = it->second;
    llvm::Type *array_type = array_alloca->getAllocatedType();

    if (!array_type->isArrayTy()) {
      scu_perror(const_cast<char *>("'%s' is not an array\n"),
                 access->array_var.name);
      return nullptr;
    }

    llvm::Type *elem_type = array_type->getArrayElementType();

    llvm::Value *index = llvm_irgen_arithmetic_expr(ctx, access->index_expr);

    if (!index)
      return nullptr;

    index = cast_to_type(ctx, index, llvm::Type::getInt32Ty(*ctx.context));

    llvm::Value *indices[] = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx.context), 0), index};

    llvm::Value *elem_ptr =
        ctx.builder->CreateGEP(array_type, array_alloca, indices, "arrayelem");

    return ctx.builder->CreateLoad(elem_type, elem_ptr, "arrayval");
  }

  case TERM_ARRAY_LITERAL: {
    scu_perror(const_cast<char *>(
                   "Array literal only valid in initialization at line %" PRIu64
                   "\n"),
               term->line);
    return nullptr;
  }

  default:
    scu_perror(const_cast<char *>("Unknown term kind %d at line %" PRIu64 "\n"),
               term->kind, term->line);
    return nullptr;
  }
}

static llvm::Value *llvm_irgen_arithmetic_expr(llvm_backend_ctx &ctx,
                                               arithmetic_expr_node *expr) {
  switch (expr->kind) {
  case AR_EXPR_INVALID:
    scu_punreachable("AR_EXPR_INVALID reached IR gen");
    break;

  case AR_EXPR_TERM:
    return llvm_irgen_term(ctx, &expr->term);

  case AR_EXPR_ADD: {
    llvm::Value *lhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.left);
    llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.right);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateAdd(lhs, rhs, "addtmp");
  }

  case AR_EXPR_SUBTRACT: {
    llvm::Value *lhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.left);
    llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.right);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateSub(lhs, rhs, "subtmp");
  }

  case AR_EXPR_MULTIPLY: {
    llvm::Value *lhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.left);
    llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.right);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateMul(lhs, rhs, "multmp");
  }

  case AR_EXPR_DIVIDE: {
    llvm::Value *lhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.left);
    llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.right);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateSDiv(lhs, rhs, "divtmp");
  }

  case AR_EXPR_MODULO: {
    llvm::Value *lhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.left);
    llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, expr->binary.right);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateSRem(lhs, rhs, "modtmp");
  }

  case AR_EXPR_UNARY_MINUS: {
    llvm::Value *operand = llvm_irgen_arithmetic_expr(ctx, expr->unary);
    if (!operand)
      return nullptr;
    return ctx.builder->CreateNeg(operand, "negtmp");
  }
  }
}

static llvm::Value *llvm_irgen_expr(llvm_backend_ctx &ctx, expr_node *expr);

static llvm::Value *llvm_irgen_relational(llvm_backend_ctx &ctx,
                                          rel_node *rel) {
  llvm::Value *lhs = llvm_irgen_term(ctx, &rel->comparison.lhs);
  llvm::Value *rhs = llvm_irgen_term(ctx, &rel->comparison.rhs);

  if (!lhs || !rhs)
    return nullptr;

  switch (rel->kind) {
  case REL_INVALID:
    scu_punreachable("REL_INVALID reached IR gen\n");
    break;

  case REL_IS_EQUAL:
    return ctx.builder->CreateICmpEQ(lhs, rhs, "cmpeq");
  case REL_NOT_EQUAL:
    return ctx.builder->CreateICmpNE(lhs, rhs, "cmpne");
  case REL_LESS_THAN:
    return ctx.builder->CreateICmpSLT(lhs, rhs, "cmplt");
  case REL_LESS_THAN_OR_EQUAL:
    return ctx.builder->CreateICmpSLE(lhs, rhs, "cmple");
  case REL_GREATER_THAN:
    return ctx.builder->CreateICmpSGT(lhs, rhs, "cmpgt");
  case REL_GREATER_THAN_OR_EQUAL:
    return ctx.builder->CreateICmpSGE(lhs, rhs, "cmpge");
  }
}

static llvm::Value *llvm_irgen_logical(llvm_backend_ctx &ctx,
                                       logical_node *log) {
  switch (log->kind) {
  case LOG_INVALID:
    scu_punreachable("LOG_INVALID reached IR gen\n");
    break;

  case LOG_AND: {
    llvm::Value *lhs = llvm_irgen_expr(ctx, log->binary.lhs);
    llvm::Value *rhs = llvm_irgen_expr(ctx, log->binary.rhs);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateAnd(lhs, rhs, "and");
  }

  case LOG_OR: {
    llvm::Value *lhs = llvm_irgen_expr(ctx, log->binary.lhs);
    llvm::Value *rhs = llvm_irgen_expr(ctx, log->binary.rhs);
    if (!lhs || !rhs)
      return nullptr;
    return ctx.builder->CreateOr(lhs, rhs, "or");
  }

  case LOG_NOT: {
    llvm::Value *operand = llvm_irgen_expr(ctx, log->unary.operand);
    if (!operand)
      return nullptr;
    return ctx.builder->CreateNot(operand, "not");
  }
  }
}

static llvm::Value *llvm_irgen_expr(llvm_backend_ctx &ctx, expr_node *expr) {
  switch (expr->kind) {
  case EXPR_INVALID:
    scu_punreachable("EXPR_INVALID reached IR gen\n");
    break;

  case EXPR_TERM:
    return llvm_irgen_term(ctx, &expr->term);

  case EXPR_LOGICAL:
    return llvm_irgen_logical(ctx, &expr->logical);

  case EXPR_RELATIONAL:
    return llvm_irgen_relational(ctx, &expr->relational);

  case EXPR_BOOL:
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*ctx.context),
                                  expr->boolean ? 1 : 0);
  }
}

static scu_result llvm_irgen_instr_declare(llvm_backend_ctx &ctx,
                                           variable *var) {
  llvm::Type *var_type = scl_type_to_llvm(ctx, var->type);

  if (var->is_array && var->dimensions > 0) {
    for (u32 i = var->dimensions; i-- > 0;) {
      var_type = llvm::ArrayType::get(var_type, var->dimension_sizes[i]);
    }
  }

  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();

  if (!fn) {
    scu_perror(const_cast<char *>(
                   "Variable declaration '%s' outside function at line %" PRIu64
                   "\n"),
               var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  llvm::AllocaInst *alloca = create_entry_block_alloca(fn, var->name, var_type);

  named_values[var->name] = alloca;

  return SCU_SUCCESS;
}

static scu_result
llvm_irgen_instr_initialize(llvm_backend_ctx &ctx,
                            initialize_variable_node *init_var) {
  variable *var = &init_var->var;
  llvm::Type *var_type = scl_type_to_llvm(ctx, var->type);

  if (var->is_array && var->dimensions > 0) {
    for (u32 i = var->dimensions; i-- > 0;) {
      var_type = llvm::ArrayType::get(var_type, var->dimension_sizes[i]);
    }
  }

  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();

  if (!fn) {
    scu_perror(
        const_cast<char *>(
            "Variable initialization '%s' outside function at line %" PRIu64
            "\n"),
        var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  llvm::AllocaInst *alloca = create_entry_block_alloca(fn, var->name, var_type);

  named_values[var->name] = alloca;

  llvm::Value *init_value;

  if (var->type == TYPE_BOOL) {
    init_value = llvm_irgen_expr(ctx, &init_var->boolean);
  } else {
    init_value = llvm_irgen_arithmetic_expr(ctx, init_var->arithmetic);
  }

  if (!init_value) {
    scu_perror(const_cast<char *>("Failed to generate intiialization "
                                  "expression for '%s' at line %" PRIu64 "\n"),
               var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  init_value = cast_to_type(ctx, init_value, var_type);

  ctx.builder->CreateStore(init_value, alloca);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_declare_array(llvm_backend_ctx &ctx,
                                                 declare_array_node *arr) {
  variable *var = &arr->var;

  llvm::Type *elem_type = scl_type_to_llvm(ctx, var->type);

  llvm::Value *size_val = nullptr;
  if (arr->size_expr) {
    size_val = llvm_irgen_arithmetic_expr(ctx, arr->size_expr);
    if (!size_val) {
      scu_perror(const_cast<char *>(
                     "Failed to evaluate array size for '%s' at line %" PRIu64
                     "\n"),
                 var->name, var->line);
      return SCU_ERR_CODEGEN;
    }
  }

  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>(
                   "Array declaration '%s' outside function at line %" PRIu64
                   "\n"),
               var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  llvm::AllocaInst *alloca = nullptr;

  u64 array_size = 0;

  if (llvm::ConstantInt *const_size =
          llvm::dyn_cast<llvm::ConstantInt>(size_val)) {
    array_size = const_size->getZExtValue();
  } else {
    scu_perror(
        const_cast<char *>("Non-constant array sizes not supported yet\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::Type *array_type = llvm::ArrayType::get(elem_type, array_size);

  alloca = create_entry_block_alloca(fn, var->name, array_type);

  if (!alloca) {
    scu_perror(const_cast<char *>(
                   "Failed to create array alloca for '%s' at line %" PRIu64
                   "\n"),
               var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  named_values[var->name] = alloca;

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_initialize_array(llvm_backend_ctx &ctx,
                                              initialize_array_node *arr) {
  variable *var = &arr->var;

  llvm::Type *elem_type = scl_type_to_llvm(ctx, var->type);

  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>(
                   "Array initialization '%s' outside function at line %" PRIu64
                   "\n"),
               var->name, var->line);
    return SCU_ERR_CODEGEN;
  }

  llvm::Value *size_val = nullptr;
  if (arr->size_expr) {
    size_val = llvm_irgen_arithmetic_expr(ctx, arr->size_expr);
    if (!size_val) {
      scu_perror(const_cast<char *>("Failed to evaluate array size for '%s'\n"),
                 var->name);
      return SCU_ERR_CODEGEN;
    }
  } else {
    size_val = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx.context),
                                      arr->literal.elements.count);
  }

  llvm::IRBuilder<> tmp_builder(&fn->getEntryBlock(),
                                fn->getEntryBlock().begin());

  u64 array_size = 0;

  if (llvm::ConstantInt *const_size =
          llvm::dyn_cast<llvm::ConstantInt>(size_val)) {
    array_size = const_size->getZExtValue();
  } else {
    scu_perror(
        const_cast<char *>("Non-constant array sizes not supported yet\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::Type *array_type = llvm::ArrayType::get(elem_type, array_size);

  llvm::AllocaInst *alloca =
      create_entry_block_alloca(fn, var->name, array_type);

  named_values[var->name] = alloca;

  for (u64 i = 0; i < arr->literal.elements.count; i++) {
    arithmetic_expr_node elem_expr;
    dynamic_array_get(&arr->literal.elements, i, &elem_expr);

    llvm::Value *elem_val = llvm_irgen_arithmetic_expr(ctx, &elem_expr);

    if (!elem_val)
      continue;

    elem_val = cast_to_type(ctx, elem_val, elem_type);

    llvm::Value *elem_ptr = ctx.builder->CreateGEP(
        elem_type, alloca,
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx.context), i),
        "array_elem_ptr");

    ctx.builder->CreateStore(elem_val, elem_ptr);
  }

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_assign(llvm_backend_ctx &ctx,
                                          assign_node *assign) {
  auto it = named_values.find(assign->identifier.name);
  if (it == named_values.end()) {
    scu_perror(const_cast<char *>("Unknown variable '%s' in assignment\n"),
               assign->identifier.name);
    return SCU_ERR_CODEGEN;
  }

  llvm::AllocaInst *var_alloca = it->second;

  llvm::Value *expr_val = llvm_irgen_arithmetic_expr(ctx, assign->expr);
  if (!expr_val) {
    scu_perror(const_cast<char *>(
                   "Failed to evaluate expression in assignment to '%s'\n"),
               assign->identifier.name);
    return SCU_ERR_CODEGEN;
  }

  llvm::Type *target_type = var_alloca->getAllocatedType();
  expr_val = cast_to_type(ctx, expr_val, target_type);

  ctx.builder->CreateStore(expr_val, var_alloca);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_assign_to_array_subscript(
    llvm_backend_ctx &ctx, assign_to_array_subscript_node *assign) {

  variable *var = &assign->var;

  auto it = named_values.find(var->name);
  if (it == named_values.end()) {
    scu_perror(const_cast<char *>("Unknown array variable '%s'\n"), var->name);
    return SCU_ERR_CODEGEN;
  }

  llvm::AllocaInst *array_alloca = it->second;
  llvm::Type *array_type = array_alloca->getAllocatedType();

  if (!array_type->isArrayTy()) {
    scu_perror(const_cast<char *>("'%s' is not an array\n"), var->name);
    return SCU_ERR_CODEGEN;
  }

  llvm::Type *elem_type = array_type->getArrayElementType();

  llvm::Value *index = llvm_irgen_arithmetic_expr(ctx, assign->index_expr);

  if (!index)
    return SCU_ERR_CODEGEN;

  index = cast_to_type(ctx, index, llvm::Type::getInt32Ty(*ctx.context));

  llvm::Value *indices[] = {
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx.context), 0), index};

  llvm::Value *elem_ptr =
      ctx.builder->CreateGEP(array_type, array_alloca, indices, "elem_ptr");

  llvm::Value *rhs = llvm_irgen_arithmetic_expr(ctx, assign->expr_to_assign);

  if (!rhs)
    return SCU_ERR_CODEGEN;

  rhs = cast_to_type(ctx, rhs, elem_type);

  ctx.builder->CreateStore(rhs, elem_ptr);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_if(llvm_backend_ctx &ctx, if_node *if_stmt) {
  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>("If statement outside function\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *merge_bb =
      llvm::BasicBlock::Create(*ctx.context, "if.end", fn);

  llvm::Value *cond_val = llvm_irgen_expr(ctx, &if_stmt->condition);
  if (!cond_val) {
    scu_perror(const_cast<char *>("Failed to generate if condition\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *then_bb =
      llvm::BasicBlock::Create(*ctx.context, "if.then", fn);

  llvm::BasicBlock *else_target;
  if (if_stmt->else_ifs.count > 0) {
    else_target = llvm::BasicBlock::Create(*ctx.context, "elif.cond.0", fn);
  } else if (if_stmt->else_) {
    else_target = llvm::BasicBlock::Create(*ctx.context, "if.else", fn);
  } else {
    else_target = merge_bb;
  }

  ctx.builder->CreateCondBr(cond_val, then_bb, else_target);

  ctx.builder->SetInsertPoint(then_bb);
  if (if_stmt->then.kind == COND_SINGLE_INSTR) {
    SCU_TRY(llvm_irgen_instr(ctx, if_stmt->then.single));
  } else {
    for (u64 i = 0; i < if_stmt->then.multi.count; i++) {
      instr_node instr;
      dynamic_array_get(&if_stmt->then.multi, i, &instr);
      SCU_TRY(llvm_irgen_instr(ctx, &instr));
    }
  }
  if (!ctx.builder->GetInsertBlock()->getTerminator()) {
    ctx.builder->CreateBr(merge_bb);
  }

  for (u64 i = 0; i < if_stmt->else_ifs.count; i++) {
    if_node elif;
    dynamic_array_get(&if_stmt->else_ifs, i, &elif);

    llvm::BasicBlock *elif_cond_bb = else_target;
    ctx.builder->SetInsertPoint(elif_cond_bb);

    llvm::Value *elif_cond = llvm_irgen_expr(ctx, &elif.condition);
    if (!elif_cond) {
      scu_perror(const_cast<char *>("Failed to generate else-if condition\n"));
      return SCU_ERR_CODEGEN;
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "elif.then.%" PRIu64, i);
    llvm::BasicBlock *elif_then_bb =
        llvm::BasicBlock::Create(*ctx.context, buf, fn);

    llvm::BasicBlock *next_target;
    if (i + 1 < if_stmt->else_ifs.count) {
      snprintf(buf, sizeof(buf), "elif.cond.%" PRIu64, i + 1);
      next_target = llvm::BasicBlock::Create(*ctx.context, buf, fn);
    } else if (if_stmt->else_) {
      next_target = llvm::BasicBlock::Create(*ctx.context, "if.else", fn);
    } else {
      next_target = merge_bb;
    }

    ctx.builder->CreateCondBr(elif_cond, elif_then_bb, next_target);

    ctx.builder->SetInsertPoint(elif_then_bb);
    if (elif.then.kind == COND_SINGLE_INSTR) {
      SCU_TRY(llvm_irgen_instr(ctx, elif.then.single));
    } else {
      for (u64 j = 0; j < elif.then.multi.count; j++) {
        instr_node instr;
        dynamic_array_get(&elif.then.multi, j, &instr);
        SCU_TRY(llvm_irgen_instr(ctx, &instr));
      }
    }
    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      ctx.builder->CreateBr(merge_bb);
    }

    else_target = next_target;
  }

  if (if_stmt->else_) {
    ctx.builder->SetInsertPoint(else_target);

    if (if_stmt->else_->kind == COND_SINGLE_INSTR) {
      SCU_TRY(llvm_irgen_instr(ctx, if_stmt->else_->single));
    } else {
      for (u64 i = 0; i < if_stmt->else_->multi.count; i++) {
        instr_node instr;
        dynamic_array_get(&if_stmt->else_->multi, i, &instr);
        SCU_TRY(llvm_irgen_instr(ctx, &instr));
      }
    }

    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      ctx.builder->CreateBr(merge_bb);
    }
  }

  ctx.builder->SetInsertPoint(merge_bb);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_match(llvm_backend_ctx &ctx,
                                         match_node *match_stmt) {
  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>("Match statement outside function\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::Value *match_val = llvm_irgen_arithmetic_expr(ctx, match_stmt->expr);
  if (!match_val) {
    scu_perror(const_cast<char *>("Failed to generate match expression\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *merge_bb =
      llvm::BasicBlock::Create(*ctx.context, "match.end", fn);

  llvm::BasicBlock *default_bb = merge_bb;

  for (u64 i = 0; i < match_stmt->cases.count; i++) {
    match_case_node case_node;
    dynamic_array_get(&match_stmt->cases, i, &case_node);
    if (case_node.kind == MATCH_CASE_DEFAULT) {
      break;
    }
  }

  for (u64 i = 0; i < match_stmt->cases.count; i++) {
    match_case_node case_node;
    dynamic_array_get(&match_stmt->cases, i, &case_node);

    char buf[64];
    snprintf(buf, sizeof(buf), "match.case.%" PRIu64, i);
    llvm::BasicBlock *case_body_bb =
        llvm::BasicBlock::Create(*ctx.context, buf, fn);

    llvm::BasicBlock *next_case_bb;
    if (i + 1 < match_stmt->cases.count) {
      snprintf(buf, sizeof(buf), "match.check.%" PRIu64, i + 1);
      next_case_bb = llvm::BasicBlock::Create(*ctx.context, buf, fn);
    } else {
      next_case_bb = default_bb;
    }

    switch (case_node.kind) {
    case MATCH_CASE_INVALID:
      scu_punreachable("MATCH_CASE_INVALID reached IR gen\n");
      break;

    case MATCH_CASE_VALUES: {
      llvm::Value *match_cond = nullptr;

      for (u64 j = 0; j < case_node.values.values.count; j++) {
        arithmetic_expr_node *expr;
        dynamic_array_get(&case_node.values.values, j, &expr);
        llvm::Value *case_val = llvm_irgen_arithmetic_expr(ctx, expr);

        llvm::Value *cmp = ctx.builder->CreateICmpEQ(match_val, case_val);

        if (match_cond) {
          match_cond = ctx.builder->CreateOr(match_cond, cmp);
        } else {
          match_cond = cmp;
        }
      }

      ctx.builder->CreateCondBr(match_cond, case_body_bb, next_case_bb);
      break;
    }

    case MATCH_CASE_RANGE: {
      llvm::Value *start_val =
          llvm_irgen_arithmetic_expr(ctx, case_node.range.start);
      llvm::Value *end_val =
          llvm_irgen_arithmetic_expr(ctx, case_node.range.end);

      llvm::Value *ge_start = ctx.builder->CreateICmpSGE(match_val, start_val);
      llvm::Value *le_end = ctx.builder->CreateICmpSLE(match_val, end_val);
      llvm::Value *in_range = ctx.builder->CreateAnd(ge_start, le_end);

      ctx.builder->CreateCondBr(in_range, case_body_bb, next_case_bb);
      break;
    }

    case MATCH_CASE_DEFAULT: {
      ctx.builder->CreateBr(case_body_bb);
      default_bb = case_body_bb;
      break;
    }
    }

    ctx.builder->SetInsertPoint(case_body_bb);
    if (case_node.body.kind == COND_SINGLE_INSTR) {
      SCU_TRY(llvm_irgen_instr(ctx, case_node.body.single));
    } else {
      for (u64 j = 0; j < case_node.body.multi.count; j++) {
        instr_node instr;
        dynamic_array_get(&case_node.body.multi, j, &instr);
        SCU_TRY(llvm_irgen_instr(ctx, &instr));
      }
    }

    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      ctx.builder->CreateBr(merge_bb);
    }

    if (i + 1 < match_stmt->cases.count) {
      ctx.builder->SetInsertPoint(next_case_bb);
    }
  }

  ctx.builder->SetInsertPoint(merge_bb);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_goto(llvm_backend_ctx &ctx,
                                        goto_node *goto_stmt) {
  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>("Goto statement outside function\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *target_bb = label_blocks[goto_stmt->label];
  if (!target_bb) {
    target_bb = llvm::BasicBlock::Create(*ctx.context, goto_stmt->label, fn);
    label_blocks[goto_stmt->label] = target_bb;
  }

  ctx.builder->CreateBr(target_bb);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_label(llvm_backend_ctx &ctx,
                                         label_node *label_stmt) {
  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>("Label outside function\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *label_bb = label_blocks[label_stmt->label];
  if (!label_bb) {
    label_bb = llvm::BasicBlock::Create(*ctx.context, label_stmt->label, fn);
    label_blocks[label_stmt->label] = label_bb;
  }

  if (!ctx.builder->GetInsertBlock()->getTerminator()) {
    ctx.builder->CreateBr(label_bb);
  }

  ctx.builder->SetInsertPoint(label_bb);

  return SCU_SUCCESS;
}

static llvm::BasicBlock *current_loop_header = nullptr;
static llvm::BasicBlock *current_loop_exit = nullptr;

static scu_result llvm_irgen_instr_loop(llvm_backend_ctx &ctx,
                                        loop_node *loop) {
  llvm::Function *fn = ctx.builder->GetInsertBlock()->getParent();
  if (!fn) {
    scu_perror(const_cast<char *>("Loop outside function\n"));
    return SCU_ERR_CODEGEN;
  }

  llvm::BasicBlock *prev_loop_header = current_loop_header;
  llvm::BasicBlock *prev_loop_exit = current_loop_exit;

  llvm::BasicBlock *loop_header =
      llvm::BasicBlock::Create(*ctx.context, "loop.header", fn);
  llvm::BasicBlock *loop_body =
      llvm::BasicBlock::Create(*ctx.context, "loop.body", fn);
  llvm::BasicBlock *loop_exit =
      llvm::BasicBlock::Create(*ctx.context, "loop.exit", fn);

  current_loop_header = loop_header;
  current_loop_exit = loop_exit;

  llvm::AllocaInst *iterator_ptr = nullptr;
  if (loop->kind == LOOP_FOR) {
    llvm::Type *i32_type = llvm::Type::getInt32Ty(*ctx.context);
    iterator_ptr =
        ctx.builder->CreateAlloca(i32_type, nullptr, loop->_for.iterator.name);

    llvm::Value *start_val =
        llvm_irgen_arithmetic_expr(ctx, loop->_for.range_start);
    ctx.builder->CreateStore(start_val, iterator_ptr);

    named_values[loop->_for.iterator.name] = iterator_ptr;
  }

  ctx.builder->CreateBr(loop_header);
  ctx.builder->SetInsertPoint(loop_header);

  switch (loop->kind) {
  case LOOP_INVALID:
    scu_punreachable("LOOP_INVALID reached IR gen\n");
    break;

  case LOOP_UNCONDITIONAL: {
    ctx.builder->CreateBr(loop_body);
    break;
  }

  case LOOP_WHILE: {
    llvm::Value *cond =
        llvm_irgen_expr(ctx, &loop->conditional.break_condition);
    if (!cond) {
      scu_perror(const_cast<char *>("Failed to generate while condition\n"));
      current_loop_header = prev_loop_header;
      current_loop_exit = prev_loop_exit;
      return SCU_ERR_CODEGEN;
    }
    ctx.builder->CreateCondBr(cond, loop_body, loop_exit);
    break;
  }

  case LOOP_DO_WHILE: {
    ctx.builder->CreateBr(loop_body);
    break;
  }

  case LOOP_FOR: {
    llvm::Value *current_val = ctx.builder->CreateLoad(
        llvm::Type::getInt32Ty(*ctx.context), iterator_ptr, "iter.val");
    llvm::Value *end_val =
        llvm_irgen_arithmetic_expr(ctx, loop->_for.range_end);
    llvm::Value *cond =
        ctx.builder->CreateICmpSLE(current_val, end_val, "for.cond");
    ctx.builder->CreateCondBr(cond, loop_body, loop_exit);
    break;
  }
  }

  ctx.builder->SetInsertPoint(loop_body);

  for (u64 i = 0; i < loop->instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&loop->instrs, i, &instr);

    SCU_TRY(llvm_irgen_instr(ctx, &instr));

    if (ctx.builder->GetInsertBlock()->getTerminator()) {
      break;
    }
  }

  if (loop->kind == LOOP_FOR) {
    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      llvm::Value *current_val = ctx.builder->CreateLoad(
          llvm::Type::getInt32Ty(*ctx.context), iterator_ptr, "iter.val");
      llvm::Value *next_val = ctx.builder->CreateAdd(
          current_val, llvm::ConstantInt::get(*ctx.context, llvm::APInt(32, 1)),
          "iter.next");
      ctx.builder->CreateStore(next_val, iterator_ptr);
      ctx.builder->CreateBr(loop_header);
    }
  } else if (loop->kind == LOOP_DO_WHILE) {
    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      llvm::Value *cond =
          llvm_irgen_expr(ctx, &loop->conditional.break_condition);
      if (cond) {
        ctx.builder->CreateCondBr(cond, loop_header, loop_exit);
      } else {
        ctx.builder->CreateBr(loop_exit);
      }
    }
  } else {
    if (!ctx.builder->GetInsertBlock()->getTerminator()) {
      ctx.builder->CreateBr(loop_header);
    }
  }

  if (loop->kind == LOOP_FOR) {
    named_values.erase(loop->_for.iterator.name);
  }

  current_loop_header = prev_loop_header;
  current_loop_exit = prev_loop_exit;

  ctx.builder->SetInsertPoint(loop_exit);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_loop_break(llvm_backend_ctx &ctx) {
  if (!current_loop_exit) {
    scu_perror(const_cast<char *>("Break statement outside loop\n"));
    return SCU_ERR_CODEGEN;
  }

  ctx.builder->CreateBr(current_loop_exit);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_loop_continue(llvm_backend_ctx &ctx) {
  if (!current_loop_header) {
    scu_perror(const_cast<char *>("Continue statement outside loop\n"));
    return SCU_ERR_CODEGEN;
  }

  ctx.builder->CreateBr(current_loop_header);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_fn_define(llvm_backend_ctx &ctx,
                                             fn_node *fn) {
  named_values.clear();
  label_blocks.clear();

  std::vector<llvm::Type *> param_types;
  for (u64 i = 0; i < fn->parameters.count; i++) {
    variable param;
    dynamic_array_get(&fn->parameters, i, &param);

    param_types.push_back(scl_type_to_llvm(ctx, param.type));
  }

  llvm::Type *return_type = llvm::Type::getVoidTy(*ctx.context);
  if (fn->returntypes.count > 0) {
    type ret_type;
    dynamic_array_get(&fn->returntypes, 0, &ret_type);

    return_type = scl_type_to_llvm(ctx, ret_type);
  }

  llvm::FunctionType *fn_type =
      llvm::FunctionType::get(return_type, param_types, fn->is_variadic);

  llvm::Function *function = ctx.module->getFunction(fn->name);
  if (!function) {
    function = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage,
                                      fn->name, ctx.module);
  }

  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(*ctx.context, "entry", function);
  ctx.builder->SetInsertPoint(entry);

  unsigned idx = 0;
  for (auto &arg : function->args()) {
    variable param;
    dynamic_array_get(&fn->parameters, idx++, &param);

    arg.setName(param.name);

    llvm::AllocaInst *alloca =
        create_entry_block_alloca(function, param.name, arg.getType());

    ctx.builder->CreateStore(&arg, alloca);

    named_values[param.name] = alloca;
  }

  for (u64 i = 0; i < fn->defined.instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&fn->defined.instrs, i, &instr);

    SCU_TRY(llvm_irgen_instr(ctx, &instr));

    if (ctx.builder->GetInsertBlock()->getTerminator()) {
      break;
    }
  }

  if (!ctx.builder->GetInsertBlock()->getTerminator()) {
    if (fn->returntypes.count == 0) {
      ctx.builder->CreateRetVoid();
    } else {
      llvm::Value *zero = llvm::Constant::getNullValue(return_type);
      ctx.builder->CreateRet(zero);
    }
  }

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_fn_declare(llvm_backend_ctx &ctx,
                                              fn_node *fn) {
  std::vector<llvm::Type *> param_types;
  for (u64 i = 0; i < fn->parameters.count; i++) {
    variable param;
    dynamic_array_get(&fn->parameters, i, &param);

    param_types.push_back(scl_type_to_llvm(ctx, param.type));
  }

  llvm::Type *return_type = llvm::Type::getVoidTy(*ctx.context);
  if (fn->returntypes.count > 0) {
    type ret_type;
    dynamic_array_get(&fn->returntypes, 0, &ret_type);

    return_type = scl_type_to_llvm(ctx, ret_type);
  }

  llvm::FunctionType *fn_type =
      llvm::FunctionType::get(return_type, param_types, fn->is_variadic);

  llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, fn->name,
                         ctx.module);

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_return(llvm_backend_ctx &ctx,
                                          return_node *ret) {
  if (ret->returnvals.count == 0) {
    ctx.builder->CreateRetVoid();
  } else {
    arithmetic_expr_node ret_expr;
    dynamic_array_get(&ret->returnvals, 0, &ret_expr);

    llvm::Value *ret_val = llvm_irgen_arithmetic_expr(ctx, &ret_expr);

    if (!ret_val) {
      scu_perror(const_cast<char *>("Failed to generate return expression\n"));
      ctx.builder->CreateRetVoid();
      return SCU_ERR_CODEGEN;
    }

    ctx.builder->CreateRet(ret_val);
  }

  return SCU_SUCCESS;
}

static scu_result llvm_irgen_instr_fn_call(llvm_backend_ctx &ctx,
                                           fn_call_node *call) {
  llvm::Function *callee = ctx.module->getFunction(call->name);

  if (!callee) {
    scu_perror(const_cast<char *>("Unknown function '%s' in call\n"),
               call->name);
    return SCU_ERR_CODEGEN;
  }

  std::vector<llvm::Value *> args;
  for (u64 i = 0; i < call->parameters.count; i++) {
    arithmetic_expr_node arg_expr;
    dynamic_array_get(&call->parameters, i, &arg_expr);

    llvm::Value *arg_val = llvm_irgen_arithmetic_expr(ctx, &arg_expr);

    if (!arg_val) {
      scu_perror(const_cast<char *>("Failed to evaluate argument %" PRIu64
                                    " in call to '%s'\n"),
                 i, call->name);
      return SCU_ERR_CODEGEN;
    }

    args.push_back(arg_val);
  }

  ctx.builder->CreateCall(callee, args);

  return SCU_SUCCESS;
}

scu_result llvm_irgen_instr(llvm_backend_ctx &ctx, instr_node *instr) {
  switch (instr->kind) {
  case INSTR_INVALID:
    scu_punreachable("INSTR_INVALID found in IR gen");
    break;

  case INSTR_DECLARE:
    SCU_TRY(llvm_irgen_instr_declare(ctx, &instr->declare_variable));
    break;

  case INSTR_INITIALIZE:
    SCU_TRY(llvm_irgen_instr_initialize(ctx, &instr->initialize_variable));
    break;

  case INSTR_DECLARE_ARRAY:
    SCU_TRY(llvm_irgen_instr_declare_array(ctx, &instr->declare_array));
    break;

  case INSTR_INITIALIZE_ARRAY:
    SCU_TRY(llvm_irgen_initialize_array(ctx, &instr->initialize_array));
    break;

  case INSTR_ASSIGN:
    SCU_TRY(llvm_irgen_instr_assign(ctx, &instr->assign));
    break;

  case INSTR_ASSIGN_TO_ARRAY_SUBSCRIPT:
    SCU_TRY(llvm_irgen_instr_assign_to_array_subscript(
        ctx, &instr->assign_to_array_subscript));
    break;

  case INSTR_IF:
    SCU_TRY(llvm_irgen_instr_if(ctx, &instr->if_));
    break;

  case INSTR_MATCH:
    SCU_TRY(llvm_irgen_instr_match(ctx, &instr->match));
    break;

  case INSTR_GOTO:
    SCU_TRY(llvm_irgen_instr_goto(ctx, &instr->goto_));
    break;

  case INSTR_LABEL:
    SCU_TRY(llvm_irgen_instr_label(ctx, &instr->label));
    break;

  case INSTR_LOOP:
    SCU_TRY(llvm_irgen_instr_loop(ctx, &instr->loop));
    break;

  case INSTR_LOOP_BREAK:
    SCU_TRY(llvm_irgen_instr_loop_break(ctx));
    break;

  case INSTR_LOOP_CONTINUE:
    SCU_TRY(llvm_irgen_instr_loop_continue(ctx));
    break;

  case INSTR_FN_DEFINE:
    SCU_TRY(llvm_irgen_instr_fn_define(ctx, &instr->fn_define_node));
    break;

  case INSTR_FN_DECLARE:
    SCU_TRY(llvm_irgen_instr_fn_declare(ctx, &instr->fn_declare_node));
    break;

  case INSTR_RETURN:
    SCU_TRY(llvm_irgen_instr_return(ctx, &instr->ret_node));
    break;

  case INSTR_FN_CALL:
    SCU_TRY(llvm_irgen_instr_fn_call(ctx, &instr->fn_call));
    break;
  }

  return SCU_SUCCESS;
}
