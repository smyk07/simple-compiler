#include <stdio.h>
#include <stdlib.h>

#include "includes/cstate.h"
#include "includes/utils.h"

#include "includes/codegen.h"
#include "includes/lexer.h"
#include "includes/parser.h"
#include "includes/semantic.h"

int main(int argc, char *argv[]) {
  cstate *state = cstate_init(argc, argv);
  if (!state) {
    fprintf(stderr, "Failed to allocate memory for compiler state\n");
    exit(1);
  }

  // Lexing
  lexer_tokenize(state->code_buffer, state->code_buffer_len, state->tokens,
                 &state->error_count);

  // Lexing test function
  if (state->debug)
    print_tokens(state->tokens);

  // Parsing
  parser_init(state->tokens, state->parser);
  parse_program(state->parser, state->program, &state->error_count);

  // Parsing test function
  if (state->debug)
    print_program(state->program);

  // Semantic Analysis
  check_semantics(&state->program->instrs, state->variables,
                  &state->error_count);

  // Semantic Debug Statements
  if (state->debug)
    scu_pdebug("Semantic Analysis Complete\n");

  // Codegen & Assembler
  program_asm(state->program, state->variables, state->extracted_filename,
              &state->error_count);

  // Codegen & Assembler Debug Statements
  if (state->debug)
    scu_pdebug("Codegen & Assembling Complete\n");

  // Restore STDOUT
  stdout = fopen("/dev/tty", "w");
  scu_psuccess("%s\n", state->filename);

  // Free memory
  fflush(stdout);
  fclose(stdout);
  cstate_free(state);

  return 0;
}
