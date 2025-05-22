/**
 * file: lexer.h
 * brief: Lexical analyzer for the simple-compiler
 */

#ifndef LEXER
#define LEXER

#include "data_structures.h"

typedef enum token_kind {
  // Keywords
  TOKEN_INPUT,
  TOKEN_OUTPUT,
  TOKEN_GOTO,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_LABEL,
  TOKEN_TYPE_INT,
  TOKEN_TYPE_CHAR,

  // Literals
  TOKEN_IDENTIFIER,
  TOKEN_INT,
  TOKEN_CHAR,

  // Operators
  TOKEN_ASSIGN,
  TOKEN_ADD,
  TOKEN_SUBTRACT,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_MODULO,
  TOKEN_IS_EQUAL,
  TOKEN_NOT_EQUAL,
  TOKEN_LESS_THAN,
  TOKEN_LESS_THAN_OR_EQUAL,
  TOKEN_GREATER_THAN,
  TOKEN_GREATER_THAN_OR_EQUAL,

  // Special Tokens
  TOKEN_NULL,
  TOKEN_INVALID,
  TOKEN_END
} token_kind;

typedef union token_value {
  int integer;
  char character;
  char *str;
} token_value;

typedef struct token {
  token_kind kind;
  token_value value;
} token;

typedef struct lexer {
  char *buffer;
  unsigned int buffer_len;
  unsigned int pos;
  unsigned int read_pos;
  char ch;
} lexer;

int lexer_tokenize(char *buffer, unsigned int buffer_len,
                   dynamic_array *tokens);

char *show_token_kind(token_kind kind);
void print_tokens(dynamic_array *tokens);

#endif
