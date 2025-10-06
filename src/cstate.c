#include "cstate.h"
#include "ast.h"
#include "ds/dynamic_array.h"
#include "ds/stack.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
    printf("--verbose      OR -v \t Print progress messages for various "
           "stages.\n");
    printf("--output       OR -o \t Specify output binary filename.\n");
    printf("--include_dir  OR -i \t Specify include directory path.\n");
    exit(1);
  }

  int i = 1;
  char *positional_filename = NULL;
  s->output_filename = NULL;
  s->include_dir = NULL;
  s->code_buffer = NULL;

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

    if (strcmp(arg, "--include_dir") == 0 || strcmp(arg, "-i") == 0) {
      if (i + 1 >= argc) {
        scu_perror(&s->error_count, "Missing directory path after %s\n", arg);
        exit(1);
      }

      if (s->include_dir != NULL) {
        scu_perror(&s->error_count,
                   "Include directory specified more than once: %s\n",
                   argv[i + 1]);
        exit(1);
      }

      struct stat st;
      if (stat(argv[i + 1], &st) != 0) {
        scu_perror(&s->error_count, "Include directory does not exist: %s\n",
                   argv[i + 1]);
        exit(1);
      }

      if (!S_ISDIR(st.st_mode)) {
        scu_perror(&s->error_count, "Path is not a directory: %s\n",
                   argv[i + 1]);
        exit(1);
      }

      s->include_dir = argv[i + 1];
      s->options.include_dir_specified = true;
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

  if (s->include_dir == NULL)
    s->include_dir = strdup(".");

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
  s->program->loop_counter = 0;

  s->loops = scu_checked_malloc(sizeof(stack));
  stack_init(s->loops, sizeof(loop_node *));

  s->variables = scu_checked_malloc(sizeof(dynamic_array));
  dynamic_array_init(s->variables, sizeof(variable));

  return s;
}

void cstate_free(cstate *s) {
  free(s->output_filename);
  free(s->code_buffer);

  free_tokens(s->tokens);
  dynamic_array_free(s->tokens);
  free(s->tokens);

  free(s->parser);

  free_if_instrs(s->program);
  free_expressions(s->program);
  free_loops(s->program);
  dynamic_array_free(&s->program->instrs);
  free(s->program);

  stack_free(s->loops);
  free(s->loops);

  dynamic_array_free(s->variables);
  free(s->variables);

  free(s);
}
