/*
 * ast: Abstract Syntax Tree implementation and node definitions.
 */

#ifndef AST
#define AST

#include "ds/dynamic_array.h"
#include "token.h"

#include <stddef.h>
#include <stdint.h>

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
  size_t stack_offset;
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
  INSTR_FASM_DEFINE,
  INSTR_FASM,
  INSTR_LOOP,
  INSTR_LOOP_BREAK,
  INSTR_LOOP_CONTINUE,
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

typedef struct fasm_define_node {
  const char *content;
} fasm_define_node;

typedef enum fasm_node_kind { ARG, NON_ARG } fasm_node_kind;

typedef struct fasm_node {
  fasm_node_kind kind;
  variable argument;
  const char *content;
} fasm_node;

typedef struct loop_node {
  size_t loop_id;
  dynamic_array instrs;
} loop_node;

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
    fasm_define_node fasm_def;
    fasm_node fasm;
    loop_node loop;
  };
} instr_node;

/*
 * @struct program_node: wrapper around a dynamic_array of instructions.
 */
typedef struct program_node {
  size_t loop_counter;
  dynamic_array instrs;
} program_node;

#endif // !AST
