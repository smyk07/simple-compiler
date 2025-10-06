/*
 * codegen: converts the simple-compiler AST to Assembly.
 */

#ifndef CODEGEN
#define CODEGEN

#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/stack.h"

/*
 * @brief: convert a dynamic_array of instructions to FASM assembly.
 *
 * @param program: basically a wrapper around a dynamic_array of instructions.
 * @param variables: dynamic_array of variables.
 * @param filename: filename needed for output file.
 * @param errors: counter variable to increment when an error is encountered.
 */
void instrs_to_asm(program_node *program, dynamic_array *variables,
                   stack *loops, const char *filename);

#endif // !CODEGEN
