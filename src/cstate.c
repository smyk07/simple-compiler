#include "cstate.h"
#include "lexer.h"
#include "token.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cstate *cstate_create_from_args(int argc, char *argv[]) {
  cstate *s = scu_checked_malloc(sizeof(cstate));

  if (argc <= 1) {
    /*
     * Default output.
     * Should list out each option and a short description.
     * Take care of wrapping after ~80 characters.
     */
    printf("Simple Compiler - Just as the name suggests\n");
    printf("Usage: sclc [OPTIONS] <filename>\n\n");
    printf("OPTIONS:\n");
    printf("--verbose OR -v \t Print progress messages for various stages.\n");
    exit(1);
  }

  int i = 1;
  char *positional_filename = NULL;

  while (i < argc) {
    char *arg = argv[i];

    if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
      s->options.verbose = true;
      i++;
      continue;
    }

    if (strcmp(arg, "--output") == 0 || strcmp(arg, "-o") == 0) {
      if (i + 1 >= argc) {
        scu_perror(&s->error_count, "Missing filename after %s\n", arg);
        exit(1);
      }

      if (s->output_filename != NULL) {
        scu_perror(&s->error_count, "Output specified more than once: %s\n",
                   argv[i + 1]);
        exit(1);
      }

      s->output_filename = argv[i + 1];
      s->options.output = true;
      i += 2;
      continue;
    }

    if (arg[0] != '-') {
      if (positional_filename != NULL) {
        scu_perror(&s->error_count, "Multiple input files specified: %s\n",
                   arg);
        exit(1);
      }

      positional_filename = arg;
      i++;
      continue;
    }

    scu_perror(&s->error_count, "Unknown option: %s\n", arg);
    exit(1);
  }

  if (positional_filename == NULL) {
    scu_perror(&s->error_count, "Missing input filename\n");
    exit(1);
  }
  s->filename = positional_filename;

  if (s->output_filename == NULL) {
    s->output_filename = scu_extract_name(s->filename);
    if (!s->output_filename) {
      scu_perror(&s->error_count, "Failed to extract filename.\n");
      exit(1);
    }
  }

  s->error_count = 0;

  s->code_buffer = NULL;
  s->code_buffer_len =
      scu_read_file(s->filename, &s->code_buffer, &s->error_count);
  if (s->code_buffer == NULL) {
    scu_perror(&s->error_count, "Failed to read file: %s\n", s->filename);
    exit(1);
  }

  s->tokens = scu_checked_malloc(sizeof(dynamic_array));
  dynamic_array_init(s->tokens, sizeof(token));

  s->parser = scu_checked_malloc(sizeof(parser));

  s->program = scu_checked_malloc(sizeof(program_node));

  s->variables = scu_checked_malloc(sizeof(dynamic_array));
  dynamic_array_init(s->variables, sizeof(variable));

  return s;
}

void cstate_free(cstate *s) {
  free(s->code_buffer);

  free_tokens(s->tokens);
  dynamic_array_free(s->tokens);
  free(s->tokens);

  free(s->parser);

  free_if_instrs(s->program);
  free_expressions(s->program);
  dynamic_array_free(&s->program->instrs);
  free(s->program);

  dynamic_array_free(s->variables);
  free(s->variables);

  free(s);
}
