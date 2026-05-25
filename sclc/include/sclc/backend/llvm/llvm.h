/*
 * llvm: acts as a link between sclc frontend and the llvm backend.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef LLVM_H
#define LLVM_H

#include "core/utils.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "sclc/cstate.h"
#include "sclc/fstate.h"

/*
 * @brief: Initializes the LLVM backend for compilation.
 *
 * @param cst: Pointer to compiler state containing global compilation context
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_init(cstate *cst);

/*
 * @brief: Compiles the parsed program to LLVM IR.
 *
 * @param cst: Pointer to compiler state containing global compilation context
 * @param fst: Pointer to file state containing the parsed program and artifacts
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_compile(cstate *cst, fstate *fst);

/*
 * @brief: LLVM Optimization passes.
 *
 * @param cst: Pointer to compiler state containing global compilation context
 * @param fst: Pointer to file state containing the compiled IR
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_optimize(cstate *cst, fstate *fst);

/*
 * @brief: Emits the compiled LLVM IR to the requested format (default is a
 * linkable output file).
 *
 * @param cst: Pointer to compiler state containing global compilation context
 * @param fst: Pointer to file state containing the compiled IR
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_emit(cstate *cst, fstate *fst);

/*
 * @brief: Cleans up LLVM backend resources and frees associated memory.
 *
 * @param cst: Pointer to compiler state containing global compilation context
 * @param fst: Pointer to file state containing LLVM backend artifacts
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_cleanup(cstate *, fstate *);

/*
 * @brief: Links the emitted object files
 *
 * @param cst: Pointer to compiler state containing global compilation context
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result llvm_backend_link(cstate *cst);

#ifdef __cplusplus
}
#endif

#endif // !LLVM_H
