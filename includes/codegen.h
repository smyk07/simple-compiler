/*
 * file: codegen.h
 * brief: Converts the simple-compiler AST to Assembly.
 */
#ifndef CODEGEN
#define CODEGEN

#include "data_structures.h"
#include "parser.h"

void program_asm(program_node *program, dynamic_array *variables);

#endif // !CODEGEN
