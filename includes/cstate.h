/*
 * cstate: Per-binary compilation state, groups all variables and options for
 * one build unit.
 *
 * Usage:
 * cstate *state = cstate_create_from_args(argc, argv);
 * ...
 * cstate_free(state);
 */

#ifndef CSTATE_H
#define CSTATE_H

#include "data_structures.h"
#include "parser.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct coptions {
  /*
   * Print progress messages for various stages.
   */
  bool verbose;

  /*
   * Write output to output_filename instead of the default filename.
   */
  bool output;
} coptions;

typedef struct cstate {
  /*
   * Stores the name of the file to be compiled.
   * Ex: main.sclc
   */
  const char *filename;

  /*
   * Name of the output file. default is extracted from filename.
   * Ex: main.sclc => main
   */
  char *output_filename;

  /*
   * Options for the compilation process.
   */
  coptions options;

  /*
   * Main error count for the whole compilation process.
   */
  unsigned int error_count;

  /*
   * Source buffer.
   */
  char *code_buffer;
  size_t code_buffer_len;

  /*
   * Variables / artifacts for the whole compiler pipeline.
   */
  dynamic_array *tokens;
  parser *parser;
  program_node *program;
  dynamic_array *variables;
} cstate;

/*
 * Create compiler state from CLI arguments.
 *
 * @param argc: count of args
 * @param argv: array of arguments (string)
 */
cstate *cstate_create_from_args(int argc, char *argv[]);

/*
 * Free / Destroy the compiler state after it is used.
 *
 * @param s: pointer to malloc'd cstate.
 */
void cstate_free(cstate *s);

#endif // !CSTATE_H
