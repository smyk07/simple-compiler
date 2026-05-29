/*
 * fstate: per-file compilation data
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "sclc/fstate.h"

#include "frontend/ast.h"
#include "frontend/var.h"

#include "core/ds/dynamic_array.h"
#include "core/ds/ht.h"
#include "core/utils.h"

#include <stdlib.h>
#include <string.h>

scu_result fstate_init(fstate *fst, const char *filepath) {
  fst->filepath_len = strlen(filepath) + 1;
  fst->filepath = scu_checked_malloc(fst->filepath_len * sizeof(char));
  strcpy(fst->filepath, filepath);

  fst->extracted_filepath = scu_extract_name(fst->filepath);

  SCU_TRY(
      scu_read_file(fst->filepath, &fst->code_buffer, &fst->code_buffer_len));

  dynamic_array_init(&fst->tokens, sizeof(token));

  ast_init(&fst->program_ast);

  dynamic_array_init(&fst->loops, sizeof(loop_node));

  ht_init(&fst->variables, sizeof(variable));

  ht_init(&fst->functions, sizeof(fn_node));

  return SCU_SUCCESS;
}

void fstate_free(fstate *fst) {
  free(fst->filepath);
  free(fst->extracted_filepath);
  free(fst->code_buffer);

  free_tokens(&fst->tokens);
  dynamic_array_free(&fst->tokens);

  ast_free(&fst->program_ast);

  dynamic_array_free(&fst->loops);

  ht_free(&fst->variables);

  ht_free(&fst->functions);
}
