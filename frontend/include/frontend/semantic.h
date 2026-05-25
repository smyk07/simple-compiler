/*
 * semantic: variable checking and type checking for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "frontend/ast.h"

#include "core/common.h"
#include "core/ds/dynamic_array.h"
#include "core/ds/ht.h"
#include "core/utils.h"

#include <stddef.h>

/*
 * @brief: evaluate a constant expression to extract integer value
 *
 * @param expr: pointer to an expr_node
 *
 * @return: the integer value of the constant expression
 */
u32 evaluate_const_expr(arithmetic_expr_node *expr);

/*
 * @brief: go through all the variables and labels in the parse tree and check
 * for any erorrs.
 *
 * @param instrs: pointer to the dynamic_array of instructions.
 * @param variables: pointer to hash table of variable.
 * @param functions: pointer to the functions hash table.
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result check_semantics(dynamic_array *instrs, ht *variables, ht *functions);

#endif // !SEMANTIC_H
