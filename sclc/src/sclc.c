/*
 * sclc: yet another systems programming language
 * Initial compiler implementation in C
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "sclc/backend/backend.h"
#include "sclc/cstate.h"
#include "sclc/fstate.h"

#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "frontend/semantic.h"

#include "core/creg.h"
#include "core/ds/dynamic_array.h"
#include "core/utils.h"

#include <stdlib.h>
#include <time.h>

CREG_CREATE(sclc)

int main(int argc, char *argv[]) {
  CREG_INIT(sclc);

  // Initialize compiler state
  static cstate cst = {0};
  creg_register(&sclc, &cst, cstate_free_shim);
  cstate_init(&cst, argc, argv);

  static backend backend = {0};
  creg_register(&sclc, &backend, backend_free_shim);
  backend_init(&backend, &cst);

  clock_t start, end;
  double time_taken;
  start = clock();

  for (u64 i = 0; i < cst.files.count; i++) {
    fstate *fst;
    dynamic_array_get(&cst.files, i, &fst);

    // Lexing
    lexer_tokenize(fst->code_buffer, fst->code_buffer_len, &fst->tokens,
                   cst.include_dir);

    // Lexing debug statements
    if (cst.options.verbose) {
      scu_pdebug("Lexing Debug Statements for %s:\n", fst->filepath);
      token_print_tokens(&fst->tokens);
    }

    // Parsing
    parser_parse_program(&fst->tokens, &fst->program_ast);

    // Parsing debug statements
    if (cst.options.verbose) {
      scu_pdebug("Parsing Debug Statements for %s:\n", fst->filepath);
      print_ast(&fst->program_ast);
    }

    // Semantic Analysis
    check_semantics(&fst->program_ast.instrs, &fst->variables, &fst->functions);

    // Semantic Debug Statement
    if (cst.options.verbose)
      scu_pdebug("Semantic Analysis Complete for %s\n", fst->filepath);

    // Initiate backend compilation
    backend_compile(&backend, &cst, fst);

    // Codegen Debug Statements
    if (cst.options.verbose)
      scu_pdebug("Codegen Complete for %s\n", fst->filepath);

    if (cst.options.verbose)
      scu_psuccess("COMPILED %s\n", fst->filepath);
  }

  if (!(cst.options.compile_only))
    backend.link(&cst);

  if (cst.options.emit_llvm || cst.options.emit_asm) {
    return 0;
  }

  end = clock();
  time_taken = (double)(end - start) / CLOCKS_PER_SEC;

  if (cst.options.verbose)
    scu_psuccess("  LINKED %s - %.2fs total time taken\n", cst.output_filepath,
                 time_taken);

  return 0;
}
