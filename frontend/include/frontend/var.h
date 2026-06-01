/*
 * var: variable struct definition for the SCULL Langauge
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef VAR_H
#define VAR_H

#include "core/common.h"
#include "core/ds/ht.h"

#include "frontend/types.h"

/*
 * @struct variable: represents a variable.
 */
typedef struct variable {
  type type;
  char *name;
  u64 line;
  u64 stack_offset;

  bool is_array;
  u64 dimensions;
  u64 *dimension_sizes;
} variable;

/*
 * @brief: check for a variable's type by its name / identifier and line data.
 *
 * @param variables: pointer to hash table of variable.
 * @param var_to_find: pointer to a variable struct which we intend to find in
 * the dynamic_array.
 *
 * @return: data type of the variable (enumeration)
 */
type get_var_type(ht *variables, variable *var_to_find);

#endif // !VAR_H
