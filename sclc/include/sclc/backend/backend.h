/*
 * backend: handles code generation, connects frontend to all backends
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef BACKEND_H
#define BACKEND_H

#include "core/creg.h"
#include "core/utils.h"

/* forward declarations of cstate and fstate to avoid recursive inclusion */
typedef struct cstate cstate;
typedef struct fstate fstate;

typedef struct backend {
  // one time setup for the underlying backend to which we are providing an
  // abstract layer
  scu_result (*setup)(cstate *cst);

  scu_result (*compile)(cstate *cst, fstate *fst);  // Compile a single file
  scu_result (*optimize)(cstate *cst, fstate *fst); // Optimize IR
  scu_result (*emit)(cstate *cst, fstate *fst);     // Emit object file
  scu_result (*cleanup)(cstate *cst,
                        fstate *fst); // cleanup file specific resources

  scu_result (*link)(cstate *cst); // link all object files
} backend;

/*
 * @brief: Initialize a new backend instance.
 *
 * @param backend: Pointer to the backend instance
 * @param cst: Pointer to compiler state containing global compilation context
 *
 * @return: Pointer to newly allocated backend
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result backend_init(backend *backend, cstate *cst);

/*
 * @brief: Compiles the parsed program using the backend.
 *
 * @param backend: Pointer to the backend instance
 * @param cst: Pointer to compiler state containing global compilation context
 * @param fst: Pointer to file state containing the parsed program and artifacts
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result backend_compile(backend *backend, cstate *cst, fstate *fst);

/*
 * @brief: Frees all memory associated with a backend instance.
 *
 * @param backend: Pointer to the backend to be freed
 */
void backend_free(backend *backend);

CREG_CLEANUP_SHIM(backend, backend_free)

#endif // !BACKEND_H
