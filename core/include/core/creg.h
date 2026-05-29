/*
 * creg: cleanup registry
 *
 * A creg is a LIFO registry of dynamically allocated objects and their
 * associated cleanup functions. Intended to be initialized with creg_init() and
 * destroyed on exit with creg_cleanup().
 *
 * Could be used per-process or per-scope
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef CREG_H
#define CREG_H

#include "core/ds/dynamic_array.h"

#include <stdlib.h>

typedef dynamic_array creg;

typedef void (*creg_cleanup_fn)(void *);

/*
 * @brief: initializes a creg object
 *
 * @param creg: pointer to a creg object
 */
void creg_init(creg *creg);

/*
 * @brief: registers an object along with its cleanup function onto the creg.
 *
 * @param creg: pointer to a creg object
 * @param obj: pointer to the object to be registered
 * @param fn: cleanup function
 */
void creg_register(creg *creg, void *obj, creg_cleanup_fn fn);

/*
 * @brief: calls all entries' cleanup functions and frees the creg object too.
 *
 * @param creg: pointer to a creg object
 */
void creg_cleanup(creg *creg);

/*
 * @brief: declares a static creg instance and its associated atexit handler.
 * Must be placed at the top of a file / file scope. Intended to be used with
 * CREG_INIT()
 *
 * @param name: the name of the creg instance
 */
#define CREG_CREATE(name)                                                      \
  static creg name;                                                            \
  static void name##_atexit(void) { creg_cleanup(&name); }

/*
 * @brief: initializes a creg instance and registers its atexit handler.
 *         must be called in main() after CREATE_CREG().
 *
 * @param name: the name of the creg instance passed to CREATE_CREG()
 */
#define CREG_INIT(name)                                                        \
  do {                                                                         \
    creg_init(&name);                                                          \
    atexit(name##_atexit);                                                     \
  } while (0)

/*
 * @brief: generates a cleanup shim function for use with creg_register. must be
 * placed at file scope.
 *
 * @param type: the type of the object to be freed
 * @param fn:   the typed cleanup function (must take a single pointer to type)
 */
#define CREG_CLEANUP_SHIM(type, fn)                                            \
  __attribute__((unused)) static void fn##_shim(void *obj) { fn((type *)obj); }

#endif // !CREG_H
