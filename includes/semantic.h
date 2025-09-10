/*
 * semantic: semantic checking for the simple-compiler. Includes variable
 * checking and type checking.
 */

#ifndef SEMANTIC
#define SEMANTIC

#include "ds/dynamic_array.h"
#include "parser.h"

/*
 * @brief: check if a certain variable exists in a dynamic_array of variables.
 *
 * @param variables: pointer to a dynamic_array of variables.
 * @param var_to_find: pointer to a variable struct which we intend to find in
 * the dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: int
 */
int find_variables(dynamic_array *variables, variable *var_to_find,
                   unsigned int *errors);

/*
 * @brief: check for a variable's type by its name / identifier and line data.
 *
 * @param name: string for variable's name / identifier.
 * @param line: line where the variable / identifier is located.
 * @param variables: pointer to dynamic_array of variable.
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: data type of the variable (enumeration)
 */
type var_type(const char *name, size_t line, dynamic_array *variables,
              unsigned int *errors);

/*
 * @brief: go through all the variables and labels in the parse tree and check
 * for any erorrs.
 *
 * @param instrs: pointer to the dynamic_array of instructions.
 * @param variables: pointer to dynamic_array of variable.
 * @param errors: counter variable to increment when an error is encountered.
 */
void check_semantics(dynamic_array *instrs, dynamic_array *variables,
                     unsigned int *errors);

#endif // !SEMANTIC
