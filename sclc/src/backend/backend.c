/*
 * backend: handles code generation, connects frontend to all backends
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "sclc/backend/backend.h"
#include "core/utils.h"
#include "sclc/backend/llvm/llvm.h"

#include <stdlib.h>

scu_result backend_init(backend *backend, cstate *cst) {
  backend->setup = llvm_backend_init;

  backend->compile = llvm_backend_compile;
  backend->emit = llvm_backend_emit;
  backend->optimize = llvm_backend_optimize;
  backend->cleanup = llvm_backend_cleanup;

  backend->link = llvm_backend_link;

  // one-time backend setup
  SCU_TRY(backend->setup(cst));

  return SCU_SUCCESS;
}

scu_result backend_compile(backend *backend, cstate *cst, fstate *fst) {
  if (backend->compile)
    SCU_TRY(backend->compile(cst, fst));

  if (backend->optimize)
    SCU_TRY(backend->optimize(cst, fst));

  if (backend->emit)
    SCU_TRY(backend->emit(cst, fst));

  if (backend->cleanup)
    SCU_TRY(backend->cleanup(cst, fst));

  return SCU_SUCCESS;
}

void backend_free(backend *backend) {
  if (!backend)
    return;

  backend->setup = NULL;
  backend->compile = NULL;
  backend->emit = NULL;
  backend->cleanup = NULL;
}
