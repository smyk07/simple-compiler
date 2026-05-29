/*
 * creg: cleanup registry
 *
 * A creg is a LIFO registry of dynamically allocated objects and their
 * associated cleanup functions. Intended to be initialized with creg_init() and
 * used once per process and destroyed on exit with creg_cleanup().
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "core/creg.h"

#include "core/ds/dynamic_array.h"

typedef struct creg_entry {
  void *obj;
  creg_cleanup_fn fn;
} creg_entry;

void creg_init(creg *creg) { dynamic_array_init(creg, sizeof(creg_entry)); }

void creg_register(creg *creg, void *obj, creg_cleanup_fn fn) {
  creg_entry entry = {obj, fn};
  dynamic_array_push(creg, &entry);
}

void creg_cleanup(creg *creg) {
  creg_entry entry = {0};
  while (creg->count != 0) {
    dynamic_array_pop(creg, &entry);
    entry.fn(entry.obj);
  }

  dynamic_array_free(creg);
}
