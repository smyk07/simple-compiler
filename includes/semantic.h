/*
 * semantic: semantic checking for the simple-compiler. Includes variable
 * checking and type checking.
 */

#ifndef SEMANTIC
#define SEMANTIC

#include "ds/dynamic_array.h"
#include "ds/ht.h"
#include <stddef.h>

/*
 * @brief: go through all the variables and labels in the parse tree and check
 * for any erorrs.
 *
 * @param instrs: pointer to the dynamic_array of instructions.
 * @param variables: pointer to hash table of variable.
 * @param errors: counter variable to increment when an error is encountered.
 */
void check_semantics(dynamic_array *instrs, ht *variables,
                     unsigned int *errors);

#endif // !SEMANTIC
