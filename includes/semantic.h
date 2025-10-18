/*
 * semantic: semantic checking for the simple-compiler. Includes variable
 * checking and type checking.
 */

#ifndef SEMANTIC
#define SEMANTIC

#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/ht.h"
#include <stddef.h>

/*
 * @brief: check if a certain variable exists in a dynamic_array of variables.
 *
 * @param variables: pointer to hash table of variables.
 * @param var_to_find: pointer to a variable struct which we intend to find in
 * the dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: int
 */
size_t find_stack_offset(ht *variables, variable *var_to_find,
                         unsigned int *errors);

/*
 * @brief: check for a variable's type by its name / identifier and line data.
 *
 * @param name: string for variable's name / identifier.
 * @param line: line where the variable / identifier is located.
 * @param variables: pointer to hash table of variable.
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: data type of the variable (enumeration)
 */
type var_type(const char *name, size_t line, ht *variables,
              unsigned int *errors);

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
