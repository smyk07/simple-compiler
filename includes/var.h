/*
 * var: Contains all variable and related declarations.
 */

#ifndef VAR
#define VAR

#include "ds/ht.h"
#include <stddef.h>

/*
 * @enum type: represents data types.
 */
typedef enum type { TYPE_INT = 0, TYPE_CHAR, TYPE_POINTER, TYPE_VOID } type;

/*
 * @struct variable: represents a variable.
 */
typedef struct variable {
  type type;
  char *name;
  size_t line;
  size_t stack_offset;
} variable;

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
size_t get_var_stack_offset(ht *variables, variable *var_to_find,
                            unsigned int *errors);

/*
 * @brief: check for a variable's type by its name / identifier and line data.
 *
 * @param variables: pointer to hash table of variable.
 * @param var_to_find: pointer to a variable struct which we intend to find in
 * the dynamic_array.
 * @param errors: counter variable to increment when an error is encountered.
 *
 * @return: data type of the variable (enumeration)
 */
type get_var_type(ht *variables, variable *var_to_find, unsigned int *errors);

#endif // !VARE
