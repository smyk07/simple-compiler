#include "lexer.h"
#include "data_structures.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char lexer_peek_char(lexer *l) {
  if (l->read_pos >= l->buffer_len) {
    return EOF;
  }

  return l->buffer[l->read_pos];
}

char lexer_read_char(lexer *l) {
  if (l->ch == '\n') {
    l->line++;
  }

  l->ch = lexer_peek_char(l);
  l->pos = l->read_pos;
  l->read_pos += 1;

  return l->ch;
}

void lexer_init(lexer *l, char *buffer, unsigned int buffer_len) {
  l->buffer = buffer;
  l->buffer_len = buffer_len;
  l->line = 1;
  l->pos = 0;
  l->read_pos = 0;
  l->ch = 0;

  lexer_read_char(l);
}

void skip_whitespaces(lexer *l) {
  while (isspace(l->ch)) {
    lexer_read_char(l);
  }
}

token lexer_next_token(lexer *l) {
  skip_whitespaces(l);

  if (l->ch == EOF) {
    lexer_read_char(l);
    return (token){.kind = TOKEN_END, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '=') {
    lexer_read_char(l);
    if (l->ch == '=') {
      lexer_read_char(l);
      return (token){
          .kind = TOKEN_IS_EQUAL, .value.str = NULL, .line = l->line};
    }
    return (token){.kind = TOKEN_ASSIGN, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '!') {
    lexer_read_char(l);
    if (l->ch == '=') {
      lexer_read_char(l);
      return (token){
          .kind = TOKEN_NOT_EQUAL, .value.str = NULL, .line = l->line};
    }
    string_slice slice = {.str = "!", .len = 1};
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    return (token){.kind = TOKEN_INVALID, .value.str = value, .line = l->line};
  }

  else if (l->ch == '+') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_ADD, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '-') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_SUBTRACT, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '*') {
    lexer_read_char(l);

    if (isalnum(l->ch) || l->ch == '_') {
      string_slice slice = {.str = l->buffer + l->pos, .len = 0};
      while (isalnum(l->ch) || l->ch == '_') {
        slice.len += 1;
        lexer_read_char(l);
      }
      char *value = NULL;
      string_slice_to_owned(&slice, &value);
      return (token){
          .kind = TOKEN_POINTER, .value.str = value, .line = l->line};
    }

    return (token){.kind = TOKEN_MULTIPLY, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '/') {
    lexer_read_char(l);
    if (l->ch == '/') {
      while (l->ch != '\n') {
        lexer_read_char(l);
      }
      return (token){.kind = TOKEN_COMMENT, .value.str = NULL, .line = l->line};
    }

    return (token){.kind = TOKEN_DIVIDE, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '%') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_MODULO, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '&') {
    lexer_read_char(l);
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    return (token){
        .kind = TOKEN_ADDRESS_OF, .value.str = value, .line = l->line};
  }

  else if (l->ch == '<') {
    lexer_read_char(l);
    if (l->ch == '=') {
      lexer_read_char(l);
      return (token){
          .kind = TOKEN_LESS_THAN_OR_EQUAL, .value.str = NULL, .line = l->line};
    }
    return (token){.kind = TOKEN_LESS_THAN, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '>') {
    lexer_read_char(l);
    if (l->ch == '=') {
      lexer_read_char(l);
      return (token){.kind = TOKEN_GREATER_THAN_OR_EQUAL,
                     .value.str = NULL,
                     .line = l->line};
    }
    return (token){
        .kind = TOKEN_GREATER_THAN, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == ':') {
    lexer_read_char(l);
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    return (token){.kind = TOKEN_LABEL, .value.str = value, .line = l->line};
  }

  else if (isdigit(l->ch)) {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isdigit(l->ch)) {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *temp = NULL;
    string_slice_to_owned(&slice, &temp);

    int value = atoi(temp);
    free(temp);

    return (token){.kind = TOKEN_INT, .value.integer = value, .line = l->line};
  }

  else if (l->ch == '\'') {
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
        return (token){
            .kind = TOKEN_INVALID, .value.character = l->ch, .line = l->line};
      }
      char_value = escaped_char;
    }
    lexer_read_char(l);
    if (l->ch != '\'') {
      return (token){
          .kind = TOKEN_INVALID, .value.character = l->ch, .line = l->line};
    }
    lexer_read_char(l);

    return (token){
        .kind = TOKEN_CHAR, .value.character = char_value, .line = l->line};
  }

  else if (isalnum(l->ch) || l->ch == '_') {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};
    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    if (strcmp(value, "input") == 0) {
      free(value);
      return (token){.kind = TOKEN_INPUT, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "output") == 0) {
      free(value);
      return (token){.kind = TOKEN_OUTPUT, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "goto") == 0) {
      free(value);
      return (token){.kind = TOKEN_GOTO, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "if") == 0) {
      free(value);
      return (token){.kind = TOKEN_IF, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "then") == 0) {
      free(value);
      return (token){.kind = TOKEN_THEN, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "int") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_TYPE_INT, .value.str = NULL, .line = l->line};
    } else if (strcmp(value, "char") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_TYPE_CHAR, .value.str = NULL, .line = l->line};
    } else {
      return (token){
          .kind = TOKEN_IDENTIFIER, .value.str = value, .line = l->line};
    }
  }

  else {
    string_slice slice = {.str = l->buffer + l->pos, .len = 1};
    char *value = NULL;
    string_slice_to_owned(&slice, &value);
    lexer_read_char(l);
    return (token){.kind = TOKEN_INVALID, .value.str = value, .line = l->line};
  }
}

int lexer_tokenize(char *buffer, unsigned int buffer_len, dynamic_array *tokens,
                   unsigned int *errors) {
  lexer lexer;
  lexer_init(&lexer, buffer, buffer_len);

  token token;
  do {
    token = lexer_next_token(&lexer);
    if (dynamic_array_append(tokens, &token) != 0) {
      scu_perror(errors, "Failed to append token to array\n");
      exit(1);
    }
  } while (token.kind != TOKEN_END);

  return 0;
}

char *show_token_kind(token_kind kind) {
  switch (kind) {
  case TOKEN_INPUT:
    return "input";
  case TOKEN_OUTPUT:
    return "output";
  case TOKEN_GOTO:
    return "goto";
  case TOKEN_IF:
    return "if";
  case TOKEN_THEN:
    return "then";
  case TOKEN_LABEL:
    return "label";
  case TOKEN_TYPE_INT:
    return "type_int";
  case TOKEN_TYPE_CHAR:
    return "type_char";
  case TOKEN_POINTER:
    return "pointer";
  case TOKEN_IDENTIFIER:
    return "identifier";
  case TOKEN_INT:
    return "int";
  case TOKEN_CHAR:
    return "char";
  case TOKEN_ASSIGN:
    return "assign";
  case TOKEN_ADD:
    return "add";
  case TOKEN_SUBTRACT:
    return "subtract";
  case TOKEN_MULTIPLY:
    return "multiply";
  case TOKEN_DIVIDE:
    return "divide";
  case TOKEN_MODULO:
    return "modulo";
  case TOKEN_ADDRESS_OF:
    return "addof";
  case TOKEN_IS_EQUAL:
    return "is_equal";
  case TOKEN_NOT_EQUAL:
    return "is_not_equal";
  case TOKEN_LESS_THAN:
    return "less_than";
  case TOKEN_LESS_THAN_OR_EQUAL:
    return "less_than_or_equal";
  case TOKEN_GREATER_THAN:
    return "greater_than";
  case TOKEN_GREATER_THAN_OR_EQUAL:
    return "greater_than_or_equal";
  case TOKEN_INVALID:
    return "invalid";
  case TOKEN_COMMENT:
    return "comment";
  case TOKEN_END:
    return "end";
  }
}

void print_tokens(dynamic_array *tokens) {
  scu_pdebug("Lexing Debug Statements:\n");

  for (unsigned int i = 0; i < tokens->count; i++) {
    token token;
    dynamic_array_get(tokens, i, &token);

    printf("[line %d] ", token.line);

    const char *kind = show_token_kind(token.kind);
    printf("%s", kind);

    switch (token.kind) {
    case TOKEN_INT:
      printf("(%d)", token.value.integer);
      break;
    case TOKEN_CHAR:
      printf("(%c)", token.value.character);
      break;
    case TOKEN_POINTER:
    case TOKEN_ADDRESS_OF:
    case TOKEN_LABEL:
    case TOKEN_IDENTIFIER:
    case TOKEN_INVALID:
      printf("(%s)", token.value.str);
      break;
    default:
      break;
    }

    printf("\n");
  }
}

void free_tokens(dynamic_array *tokens) {
  for (unsigned int i = 0; i < tokens->count; i++) {
    token *token = tokens->items + (i * tokens->item_size);
    if (token->kind == TOKEN_IDENTIFIER || token->kind == TOKEN_LABEL ||
        token->kind == TOKEN_INVALID || token->kind == TOKEN_ADDRESS_OF ||
        token->kind == TOKEN_POINTER) {
      free(token->value.str);
    }
  }
}
