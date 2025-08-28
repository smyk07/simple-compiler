/*
 * parser: Parser for the simple-compiler
 */

#ifndef PARSER
#define PARSER

#include "data_structures.h"
#include "lexer.h"
#include <stddef.h>

/*
 * @enum type: represents data types.
 */
typedef enum type { TYPE_INT = 0, TYPE_CHAR, TYPE_POINTER, TYPE_VOID } type;

/*
 * @struct variable: represents a variable.
 */
typedef struct variable {
  type type;
  char *name;
  size_t line;
} variable;

/*
 * @enum term_kind: enumeration of all the terms supported by the parser.
 */
typedef enum term_kind {
  TERM_INPUT = 0,
  TERM_INT,
  TERM_CHAR,
  TERM_IDENTIFIER,
  TERM_POINTER,
  TERM_DEREF,
  TERM_ADDOF
} term_kind;

/*
 * @struct term_node: represents a term.
 */
typedef struct term_node {
  term_kind kind;
  size_t line;
  union {
    token_value value;
    variable identifier;
  };
} term_node;

/*
 * @struct term_binary_node: represents a binary term.
 */
typedef struct term_binary_node {
  term_node lhs;
  term_node rhs;
} term_binary_node;

/*
 * @enum expr_kind: enumeration of all the expressions supported by the parser.
 */
typedef enum expr_kind {
  EXPR_TERM = 0,
  EXPR_ADD,
  EXPR_SUBTRACT,
  EXPR_MULTIPLY,
  EXPR_DIVIDE,
  EXPR_MODULO
} expr_kind;

/*
 * @struct expr_node: represents an expression.
 */
typedef struct expr_node {
  expr_kind kind;
  size_t line;
  union {
    term_node term;
    struct {
      struct expr_node *left;
      struct expr_node *right;
    } binary;
  };
} expr_node;

/*
 * @enum rel_kind: enumeration of all the relational operations supported by the
 * parser.
 */
typedef enum rel_kind {
  REL_IS_EQUAL = 0,
  REL_NOT_EQUAL,
  REL_LESS_THAN,
  REL_LESS_THAN_OR_EQUAL,
  REL_GREATER_THAN,
  REL_GREATER_THAN_OR_EQUAL,
} rel_kind;

/*
 * @struct rel_node: represents a relational expression.
 */
typedef struct rel_node {
  rel_kind kind;
  size_t line;
  union {
    term_binary_node is_equal;
    term_binary_node not_equal;
    term_binary_node less_than;
    term_binary_node less_than_or_equal;
    term_binary_node greater_than;
    term_binary_node greater_than_or_equal;
  };
} rel_node;

/*
 * @enum instr_kind: enumeration of all the instructions supported by the
 * parser.
 */
typedef enum instr_kind {
  INSTR_DECLARE = 0,
  INSTR_INITIALIZE,
  INSTR_ASSIGN,
  INSTR_IF,
  INSTR_GOTO,
  INSTR_OUTPUT,
  INSTR_LABEL,
} instr_kind;

/*
 * @struct instr_node: represents an instruction. (declaration)
 *
 * all the child structs for each instruction defined below.
 */
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
  const char *label;
} goto_node;

typedef struct output_node {
  term_node term;
} output_node;

typedef struct label_node {
  const char *label;
} label_node;

/*
 * @struct instr_node: represents an instruction. (definition)
 */
typedef struct instr_node {
  instr_kind kind;
  size_t line;
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

/*
 * @struct program_node: wrapper around a dynamic_array of instructions.
 */
typedef struct program_node {
  dynamic_array instrs;
} program_node;

/*
 * @struct parser: represents the parser state.
 */
typedef struct parser {
  dynamic_array tokens;
  size_t index;
} parser;

/*
 * @brief: Initializes the parser struct.
 *
 * @param tokens: pointer to dynamic_array of tokens.
 * @param p: pointer to an uninitialized parser struct.
 */
void parser_init(dynamic_array *tokens, parser *p);

/*
 * @brief: parses a dynamic_array of tokens into an AST.
 *
 * @param p: pointer to an uninitialized parser struct.
 * @param program: pointer to a program_node (empty dynamic_array of
 * instructions).
 * @param errors: error counter variable to be incremented whenever an error is
 * encountered.
 */
void parser_parse_program(parser *p, program_node *program,
                          unsigned int *errors);

/*
 * @brief: prints the whole AST (all instructions).
 *
 * @param program: pointer to a program_node (empty dynamic_array of
 * instructions).
 */
void parser_print_program(program_node *program);

/*
 * @brief: free / destroy if instructions before termination. This function is
 * needed due to there being malloc'd instructions inside those instructions.
 *
 * @param program: pointer to a program_node (empty dynamic_array of
 * instructions).
 */
void free_if_instrs(program_node *program);

/*
 * @brief: free / destroy assign and initialize instructions due to there being
 * malloc'd expressions inside those instructions.
 *
 * @param program: pointer to a program_node (empty dynamic_array of
 * instructions).
 */
void free_expressions(program_node *program);

#endif
