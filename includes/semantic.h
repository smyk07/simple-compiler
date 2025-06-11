/*
 * file: semantic.h
 * brief: Includes functions for variable tracking and typechecking for the
 * simple-compiler.
 */
#ifndef SEMANTIC
#define SEMANTIC

#include "data_structures.h"
#include "parser.h"

int find_variables(dynamic_array *variables, variable *var_to_find);
token_kind var_type(char *name, dynamic_array *variables);

void check_semantics(dynamic_array *instrs, dynamic_array *variables,
                     unsigned int *errors);

#endif // !SEMANTIC
