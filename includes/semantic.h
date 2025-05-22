/*
 * file: semantic.h
 * brief: Includes functions for variable tracking and typechecking for the
 * simple-compiler.
 */
#ifndef SEMANTIC
#define SEMANTIC

#include "data_structures.h"
#include "parser.h"

// Variable tracking
int find_variables(dynamic_array *variables, variable *var_to_find);
void instr_check_variables(instr_node *instr, dynamic_array *variables);

// Typechecking
void instr_typecheck(instr_node *instr, dynamic_array *variables);
token_kind var_type(char *name, dynamic_array *variables);

#endif // !SEMANTIC
