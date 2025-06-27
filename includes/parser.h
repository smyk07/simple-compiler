/*
 * file: parser.h
 * brief: Parser for the simple-compiler
 */
#ifndef PARSER
#define PARSER

#include "data_structures.h"
#include "lexer.h"

// Variable
typedef enum type { TYPE_INT, TYPE_CHAR, TYPE_POINTER, TYPE_VOID } type;

typedef struct variable {
  type type;
  char *name;
  unsigned int line;
} variable;

// TERM
typedef enum term_kind {
  TERM_INPUT,
  TERM_INT,
  TERM_CHAR,
  TERM_IDENTIFIER,
  TERM_POINTER,
  TERM_DEREF,
  TERM_ADDOF
} term_kind;

typedef struct term_node {
  term_kind kind;
  unsigned int line;
  union {
    token_value value;
    variable identifier;
  };
} term_node;

typedef struct term_binary_node {
  term_node lhs;
  term_node rhs;
} term_binary_node;

// EXPR
typedef enum expr_kind {
  EXPR_TERM,
  EXPR_ADD,
  EXPR_SUBTRACT,
  EXPR_MULTIPLY,
  EXPR_DIVIDE,
  EXPR_MODULO
} expr_kind;

typedef struct expr_node {
  expr_kind kind;
  unsigned int line;
  union {
    term_node term;
    struct {
      struct expr_node *left;
      struct expr_node *right;
    } binary;
  };
} expr_node;

// REL
typedef enum rel_kind {
  REL_IS_EQUAL,
  REL_NOT_EQUAL,
  REL_LESS_THAN,
  REL_LESS_THAN_OR_EQUAL,
  REL_GREATER_THAN,
  REL_GREATER_THAN_OR_EQUAL,
} rel_kind;

typedef struct rel_node {
  rel_kind kind;
  unsigned int line;
  union {
    term_binary_node is_equal;
    term_binary_node not_equal;
    term_binary_node less_than;
    term_binary_node less_than_or_equal;
    term_binary_node greater_than;
    term_binary_node greater_than_or_equal;
  };
} rel_node;

// INSTR
typedef enum instr_kind {
  INSTR_DECLARE,
  INSTR_INITIALIZE,
  INSTR_ASSIGN,
  INSTR_IF,
  INSTR_GOTO,
  INSTR_OUTPUT,
  INSTR_LABEL,
} instr_kind;

typedef struct instr_node instr_node;

typedef struct initialize_variable_node {
  variable var;
  expr_node expr;
} initialize_variable_node;

typedef struct assign_node {
  variable identifier;
  expr_node expr;
} assign_node;

typedef struct if_node {
  rel_node rel;
  instr_node *instr;
} if_node;

typedef struct goto_node {
  char *label;
} goto_node;

typedef struct output_node {
  term_node term;
} output_node;

typedef struct label_node {
  char *label;
} label_node;

typedef struct instr_node {
  instr_kind kind;
  unsigned int line;
  union {
    variable declare_variable;
    initialize_variable_node initialize_variable;
    assign_node assign;
    if_node if_;
    goto_node goto_;
    output_node output;
    label_node label;
  };
} instr_node;

typedef struct program_node {
  dynamic_array instrs;
} program_node;

typedef struct parser {
  dynamic_array tokens;
  unsigned int index;
} parser;

void parser_init(dynamic_array *tokens, parser *p);

void parse_program(parser *p, program_node *program, unsigned int *errors);

void print_program(program_node *program);

void free_if_instrs(program_node *program);

void free_expressions(program_node *program);

#endif
