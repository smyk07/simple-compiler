/*
 * llvm_irgen: generates LLVM IR from the passed scull AST.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef LLVM_IRGEN_H
#define LLVM_IRGEN_H

extern "C" {
#include "core/utils.h"
#include "frontend/ast.h"
}

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Target/TargetMachine.h>

/*
 * @struct llvm_backend_ctx: LLVM backend context containing IR generation state
 */
typedef struct llvm_backend_ctx {
  llvm::LLVMContext *context;
  llvm::Module *module;
  llvm::IRBuilder<> *builder;
  llvm::TargetMachine *target_machine;
  std::string target_triple;
} llvm_backend_ctx;

/*
 * @brief: Clears the symbol table used during IR generation.
 */
void llvm_irgen_clear_symbol_table();

/*
 * @brief: Generates LLVM IR for a single instruction node.
 *
 * @param ctx: Reference to LLVM backend context
 * @param instr: Pointer to the instruction node to generate IR for
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_irgen_instr(llvm_backend_ctx &ctx, instr_node *instr);

#endif // !LLVM_IRGEN_H
