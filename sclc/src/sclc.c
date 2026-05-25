/*
 * sclc: yet another systems programming language
 * Initial compiler implementation in C
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "core/utils.h"
#include "sclc/backend/backend.h"
#include "sclc/cstate.h"

#include "core/creg.h"

#include <stdlib.h>

CREG_CREATE(sclc)

int main(int argc, char *argv[]) {
  CREG_INIT(sclc);

  static cstate cst = {0};
  creg_register(&sclc, &cst, cstate_free_shim);
  SCU_TRY(cstate_init(&cst, argc, argv));

  return cstate_compile(&cst);
}
