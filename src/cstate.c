#include "cstate.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cstate *cstate_init(int argc, char *argv[]) {
  cstate *s = malloc(sizeof(cstate));

  s->debug = 0;
  s->error_count = 0;

  if (argc > 1) {
    if (strcmp(argv[1], "--cdebug") == 0 || strcmp(argv[1], "-cd") == 0) {
      if (argc > 2) {
        s->debug = 1;
        s->filename = argv[2];
      } else {
        scu_perror(&s->error_count, "Missing filename after %s\n", argv[1]);
        exit(1);
      }
    } else {
      s->filename = argv[1];
    }
  } else {
    printf("Simple Compiler - Just as the name suggests\n");
    printf("Usage: ./compiler [OPTIONS] <filename>\n\n");
    printf("OPTIONS:\n");
    printf("--cdebug OR -cd \t Run in compiler debug mode - prints lexer\n");
    printf("                \t and parser debug statements.\n");
    exit(1);
  }
  s->extracted_filename = scu_extract_name(s->filename);
  if (!s->extracted_filename) {
    scu_perror(&s->error_count, "Failed to extract filename.\n");
    exit(1);
  }

  s->code_buffer = NULL;
  s->code_buffer_len =
      scu_read_file(s->filename, &s->code_buffer, &s->error_count);
  if (s->code_buffer == NULL) {
    scu_perror(&s->error_count, "Failed to read file: %s\n", s->filename);
    exit(1);
  }

  s->tokens = malloc(sizeof(dynamic_array));
  dynamic_array_init(s->tokens, sizeof(token));

  s->parser = malloc(sizeof(parser));

  s->program = malloc(sizeof(program_node));

  s->variables = malloc(sizeof(dynamic_array));
  dynamic_array_init(s->variables, sizeof(variable));

  return s;
}

void cstate_free(cstate *s) {
  free(s->extracted_filename);

  free(s->code_buffer);

  free_tokens(s->tokens);
  dynamic_array_free(s->tokens);
  free(s->tokens);

  free(s->parser);

  free_if_instrs(s->program);
  dynamic_array_free(&s->program->instrs);
  free(s->program);

  dynamic_array_free(s->variables);
  free(s->variables);

  free(s);
}
