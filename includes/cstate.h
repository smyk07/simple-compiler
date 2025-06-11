/*
 * file: cstate.h
 * brief: State for the compiler, so that all variables are in one place.
 */
#ifndef CSTATE
#define CSTATE

#include "data_structures.h"
#include "parser.h"

typedef struct cstate {
  char *filename;
  char *extracted_filename;

  int debug;
  unsigned int error_count;

  char *code_buffer;
  unsigned int code_buffer_len;

  dynamic_array *tokens;
  parser *parser;
  program_node *program;
  dynamic_array *variables;
} cstate;

cstate *cstate_init(int argc, char *argv[]);
void cstate_free(cstate *s);

#endif // !CSTATE
