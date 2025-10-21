/*
 * sclc: a simple compiler, don't have a specific goal for it yet, just to
 * practice my programming compiler design and development skills.
 *
 * Copyright (C) 2025 Samyak Bambole <bambole@duck.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "codegen.h"
#include "cstate.h"
#include "lexer.h"
#include "semantic.h"
#include "utils.h"

#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
  // Initialize compiler state
  cstate *state = cstate_create_from_args(argc, argv);

  clock_t start, end;
  double time_taken;
  start = clock();

  // Lexing
  lexer_tokenize(state->code_buffer, state->code_buffer_len, state->tokens,
                 state->include_dir, &state->error_count);

  // Lexing test function
  if (state->options.verbose)
    lexer_print_tokens(state->tokens);

  // Parsing
  parser_init(state->tokens, state->parser);
  parser_parse_program(state->parser, state->program, &state->error_count);

  // Parsing test function
  if (state->options.verbose)
    parser_print_program(state->program);

  // Semantic Analysis
  check_semantics(&state->program->instrs, state->variables,
                  &state->error_count);

  // Semantic Debug Statements
  if (state->options.verbose)
    scu_pdebug("Semantic Analysis Complete\n");

  // Codegen & Assembler
  instrs_to_asm(state->program, state->variables, state->loops,
                state->output_filename, &state->error_count);

  end = clock();
  time_taken = (double)(end - start) / CLOCKS_PER_SEC;

  // Restore STDOUT
  stdout = fopen("/dev/tty", "w");

  scu_psuccess("%.2fs %s\n", time_taken, state->filename);

  // Codegen & Assembler Debug Statements
  if (state->options.verbose)
    scu_pdebug("Codegen & Assembling Complete\n");

  // Free memory
  fflush(stdout);
  fclose(stdout);
  cstate_free(state);

  return 0;
}
