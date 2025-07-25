#include "cstate.h"
#include "utils.h"

#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
  // Initialize compiler state
  cstate *state = cstate_init(argc, argv);

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

  // Restore STDOUT
  stdout = fopen("/dev/tty", "w");
  scu_psuccess("%s\n", state->filename);

  // Codegen & Assembler Debug Statements
  if (state->debug)
    scu_pdebug("Codegen & Assembling Complete\n");

  // Free memory
  fflush(stdout);
  fclose(stdout);
  cstate_free(state);

  return 0;
}
