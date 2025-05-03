#ifndef PARSER
#define PARSER

#include "data_structures.h"
#include "lexer.h"

typedef struct program_node {
  dynamic_array instrs;
} program_node;

typedef struct parser {
  dynamic_array tokens;
  unsigned int index;
} parser;

void parser_init(dynamic_array tokens, parser *p);

void parse_program(parser *p, program_node *program);

void print_program(program_node *program);

#endif
