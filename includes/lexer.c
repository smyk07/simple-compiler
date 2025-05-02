#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"

const char *show_token_kind(token_kind kind) {
  switch (kind) {
  case IDENTIFIER:
    return "identifier";
  case INPUT:
    return "input";
  case OUTPUT:
    return "output";
  case GOTO:
    return "goto";
  case IF:
    return "if";
  case THEN:
    return "then";
  case LABEL:
    return "label";
  case INT:
    return "int";
  case ASSIGN:
    return "assign";
  case ADD:
    return "add";
  case SUBTRACT:
    return "subtract";
  case LESS_THAN:
    return "less_than";
  case GREATER_THAN:
    return "greater_than";
  case INVALID:
    return "invalid";
  case END:
    return "end";
  }
}

char lexer_peek_char(lexer *l) {
  if (l->read_pos >= l->buffer_len) {
    return EOF;
  }

  return l->buffer[l->read_pos];
}

char lexer_read_char(lexer *l) {
  l->ch = lexer_peek_char(l);

  l->pos = l->read_pos;
  l->read_pos += 1;

  return l->ch;
}

void skip_whitespaces(lexer *l) {
  while (isspace(l->ch)) {
    lexer_read_char(l);
  }
}

void lexer_init(lexer *l, char *buffer, unsigned int buffer_len) {
  l->buffer = buffer;
  l->buffer_len = buffer_len;
  l->pos = 0;
  l->read_pos = 0;
  l->ch = 0;

  lexer_read_char(l);
}

token lexer_next_token(lexer *l) {
  skip_whitespaces(l);

  if (l->ch == EOF) {
    lexer_read_char(l);
    return (token){.kind = END, .value = NULL};
  } else if (l->ch == '=') {
    lexer_read_char(l);
    return (token){.kind = ASSIGN, .value = NULL};
  } else if (l->ch == '+') {
    lexer_read_char(l);
    return (token){.kind = ADD, .value = NULL};
  } else if (l->ch == '-') {
    lexer_read_char(l);
    return (token){.kind = SUBTRACT, .value = NULL};
  } else if (l->ch == '<') {
    lexer_read_char(l);
    return (token){.kind = LESS_THAN, .value = NULL};
  } else if (l->ch == '>') {
    lexer_read_char(l);
    return (token){.kind = GREATER_THAN, .value = NULL};
  } else if (l->ch == ':') {
    lexer_read_char(l);
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    return (token){.kind = LABEL, .value = value};
  } else if (isdigit(l->ch)) {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isdigit(l->ch)) {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    return (token){.kind = INT, .value = value};
  } else if (isalnum(l->ch) || l->ch == '_') {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    if (strcmp(value, "input") == 0)
      return (token){.kind = INPUT, value = NULL};
    else if (strcmp(value, "output") == 0)
      return (token){.kind = INPUT, value = NULL};
    else if (strcmp(value, "goto") == 0)
      return (token){.kind = GOTO, value = NULL};
    else if (strcmp(value, "if") == 0)
      return (token){.kind = IF, value = NULL};
    else if (strcmp(value, "then") == 0)
      return (token){.kind = THEN, value = NULL};
    else
      return (token){.kind = IDENTIFIER, value = value};
  } else {
    string_slice slice = {.str = l->buffer + l->pos, .len = 1};
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    lexer_read_char(l);
    return (token){.kind = INVALID, .value = value};
  }
}

int lexer_tokenize(char *buffer, unsigned int buffer_len,
                   dynamic_array *tokens) {
  lexer lexer;
  lexer_init(&lexer, buffer, buffer_len);

  token token;
  do {
    token = lexer_next_token(&lexer);
    if (dynamic_array_append(tokens, &token) != 0) {
      perror("Failed to append token to array...\n");
      exit(1);
    }
  } while (token.kind != END);

  return 0;
}

void print_token(token token) {
  const char *kind = show_token_kind(token.kind);
  printf("%s", kind);
  if (token.value != NULL) {
    printf("(%s)", token.value);
  }
  printf("\n");
}
