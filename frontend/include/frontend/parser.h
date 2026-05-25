/*
 * parser: recursive-descent parser for the SCULL language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef PARSER_H
#define PARSER_H

#include "core/ds/dynamic_array.h"
#include "core/utils.h"

#include "frontend/ast.h"

/*
 * @brief: parses a dynamic_array of tokens into an AST.
 *
 * @param tokens: pointer to dynamic_array of tokens.
 * @param program: pointer to a program_node (empty dynamic_array of
 * instructions).
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result parser_parse_program(dynamic_array *tokens, ast *program);

#endif // !PARSER_H
