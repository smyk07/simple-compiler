#include "lexer.h"

#include <ctype.h>
#include <stdatomic.h>
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
  case OUTPUTI:
    return "outputi";
  case OUTPUTC:
    return "outputc";
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
  case CHAR:
    return "char";
  case ASSIGN:
    return "assign";
  case ADD:
    return "add";
  case SUBTRACT:
    return "subtract";
  case MULTIPLY:
    return "multiply";
  case DIVIDE:
    return "divide";
  case MODULO:
    return "modulo";
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
  } else if (l->ch == '*') {
    lexer_read_char(l);
    return (token){.kind = MULTIPLY, .value = NULL};
  } else if (l->ch == '/') {
    lexer_read_char(l);
    return (token){.kind = DIVIDE, .value = NULL};
  } else if (l->ch == '%') {
    lexer_read_char(l);
    return (token){.kind = MODULO, .value = NULL};
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
  } else if (l->ch == '\'') {
    lexer_read_char(l);
    char char_value = l->ch;
    char escaped_char;
    if (l->ch == '\\') {
      lexer_read_char(l);
      switch (l->ch) {
      case 'n':
        escaped_char = '\n';
        break;
      case 't':
        escaped_char = '\t';
        break;
      case 'r':
        escaped_char = '\r';
        break;
      case '\\':
        escaped_char = '\\';
        break;
      case '\'':
        escaped_char = '\'';
        break;
      case '0':
        escaped_char = '\0';
        break;
      default:
        return (token){.kind = INVALID, .value = &(l->ch)};
      }
      char_value = escaped_char;
    }
    lexer_read_char(l);
    if (l->ch != '\'') {
      return (token){.kind = INVALID, .value = &(l->ch)};
    }
    lexer_read_char(l);
    char *value = (char *)malloc(2);
    if (value == NULL) {
      perror("Failed to allocate memory for character literal");
      exit(1);
    }
    value[0] = char_value;
    value[1] = '\0';
    return (token){.kind = CHAR, .value = value};
  } else if (isalnum(l->ch) || l->ch == '_') {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    if (strcmp(value, "input") == 0)
      return (token){.kind = INPUT, .value = NULL};
    else if (strcmp(value, "outputi") == 0)
      return (token){.kind = OUTPUTI, .value = NULL};
    else if (strcmp(value, "outputc") == 0)
      return (token){.kind = OUTPUTC, .value = NULL};
    else if (strcmp(value, "goto") == 0)
      return (token){.kind = GOTO, .value = NULL};
    else if (strcmp(value, "if") == 0)
      return (token){.kind = IF, .value = NULL};
    else if (strcmp(value, "then") == 0)
      return (token){.kind = THEN, .value = NULL};
    else
      return (token){.kind = IDENTIFIER, .value = value};
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

void print_tokens(dynamic_array *tokens) {
  for (unsigned int i = 0; i <= tokens->count - 1; i++) {
    token token;
    dynamic_array_get(tokens, i, &token);
    print_token(token);
  }
}
