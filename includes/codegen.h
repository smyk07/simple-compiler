/*
 * codegen: converts the simple-compiler AST to Assembly.
 */

#ifndef CODEGEN
#define CODEGEN

#include "ast.h"
#include "ds/ht.h"
#include "ds/stack.h"

/*
 * @brief: convert a dynamic_array of instructions to FASM assembly.
 *
 * @param program: basically a wrapper around a dynamic_array of instructions.
 * @param variables: hash table of variables.
 * @param filename: filename needed for output file.
 * @param errors: counter variable to increment when an error is encountered.
 */
void instrs_to_asm(program_node *program, ht *variables, stack *loops,
                   const char *filename);

#endif // !CODEGEN
