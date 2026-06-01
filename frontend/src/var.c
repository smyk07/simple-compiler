/*
 * var: variable, symbol and types for the SCULL Langauge
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "frontend/var.h"

#include "core/utils.h"

#include <inttypes.h>

type get_var_type(ht *variables, variable *var_to_find) {
  if (!variables || !var_to_find || !var_to_find->name)
    return -1;

  variable *var = ht_search(variables, var_to_find->name);

  if (!var) {
    scu_perror("Use of undeclared variable: %s [line %" PRIu64 "]\n",
               var_to_find->name, var_to_find->line);
    return -1;
  }

  return var->type;
}
