#ifndef LEXER
#define LEXER

#include "data_structures.h"

typedef enum token_kind {
  IDENTIFIER,
  INT,
  INPUT,
  OUTPUT,
  GOTO,
  IF,
  THEN,
  LABEL,
  ASSIGN,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  MODULO,
  LESS_THAN,
  GREATER_THAN,
  INVALID,
  END
} token_kind;

typedef struct token {
  token_kind kind;
  char *value;
} token;

typedef struct lexer {
  char *buffer;
  unsigned int buffer_len;
  unsigned int pos;
  unsigned int read_pos;
  char ch;
} lexer;

const char *show_token_kind(token_kind kind);
void print_token(token token);
int lexer_tokenize(char *buffer, unsigned int buffer_len,
                   dynamic_array *tokens);

#endif
