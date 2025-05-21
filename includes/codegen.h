/*
 * file: codegen.h
 * brief: Converts the simple-compiler AST to Assembly.
 */
#ifndef CODEGEN
#define CODEGEN

#include "parser.h"

void program_asm(program_node *program);

#endif // !CODEGEN
