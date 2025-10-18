#include "lexer.h"
#include "ds/dynamic_array.h"
#include "token.h"
#include "utils.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * @struct string_slice: represents a slice of strings with a specified length,
 * does not need null termination.
 */
typedef struct string_slice {
  const char *str;
  size_t len;
} string_slice;

/*
 * @brief: Converts a string_slice to a null terminated owned string.
 *
 * @param ss: pointer to a string_slice.
 * @param str: pointer to a char pointer where the allocated string will be
 * stored.
 */
static int string_slice_to_owned(string_slice *ss, char **str) {
  if (!ss || !ss->str || !str)
    return -1;

  *str = (char *)scu_checked_malloc(ss->len + 1);
  if (!*str)
    return -1;

  memcpy(*str, ss->str, ss->len);
  (*str)[ss->len] = '\0';

  return 0;
}

/*
 * @brief: Read the next character.
 *
 * @param l: pointer to lexer struct object.
 */
static char lexer_read_char(lexer *l);

/*
 * Initialize Lexer
 *
 * @param l: pointer to lexer struct object.
 * @param buffer: const char* which is the source buffer to be lexed.
 * @param buffer_len: size of buffer.
 */
static void lexer_init(lexer *l, const char *buffer, size_t buffer_len) {
  l->buffer = buffer;
  l->buffer_len = buffer_len;
  l->line = 1;
  l->pos = 0;
  l->read_pos = 0;
  l->ch = 0;

  lexer_read_char(l);
}

/*
 * @brief: Peek ahead
 *
 * @param l: pointer to lexer struct object.
 */
static char lexer_peek_char(lexer *l) {
  if (l->read_pos >= l->buffer_len) {
    return EOF;
  }

  return l->buffer[l->read_pos];
}

/*
 * @brief: Read and return the character at read position.
 *
 * @param l: pointer to lexer struct object.
 */
static char lexer_read_char(lexer *l) {
  if (l->ch == '\n') {
    l->line++;
  }

  l->ch = lexer_peek_char(l);
  l->pos = l->read_pos;
  l->read_pos += 1;

  return l->ch;
}

/*
 * @brief: Move lexer forward until it encounters another character.
 *
 * @param l: pointer to lexer struct object.
 */
static void skip_whitespaces(lexer *l) {
  while (isspace(l->ch)) {
    lexer_read_char(l);
  }
}

/*
 * @brief: Scans the buffer ahead and returns the next token.
 *
 * @param l: pointer to lexer struct object.
 */
static token lexer_next_token(lexer *l) {
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

  else if (l->ch == '(') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_LPAREN, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == ')') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_RPAREN, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '{') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_LBRACE, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '}') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_RBRACE, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '[') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_LSQBR, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == ']') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_RSQBR, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == ',') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_COMMA, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '-') {
    lexer_read_char(l);
    if (l->ch == '-') {
      while (l->ch != '\n') {
        lexer_read_char(l);
      }
      return (token){.kind = TOKEN_COMMENT, .value.str = NULL, .line = l->line};
    } else if (l->ch == '*') {
      while (l->ch != '\0') {
        if (l->ch == '*') {
          lexer_read_char(l);
          if (l->ch == '-') {
            lexer_read_char(l);
            return (token){
                .kind = TOKEN_COMMENT, .value.str = NULL, .line = l->line};
          }
          continue;
        } else {
          lexer_read_char(l);
          continue;
        }
        return (token){
            .kind = TOKEN_INVALID, .value.character = l->ch, .line = l->line};
      }
    } else if (isalnum(l->ch)) {
      string_slice slice = {.str = l->buffer + l->pos, .len = 0};
      while (isalnum(l->ch) || l->ch == '_') {
        slice.len += 1;
        lexer_read_char(l);
      }
      char *directive = NULL;
      string_slice_to_owned(&slice, &directive);
      if (strcmp(directive, "include") == 0) {
        free(directive);
        return (token){
            .kind = TOKEN_PDIR_INCLUDE, .value.str = NULL, .line = l->line};
      }
    }
    return (token){.kind = TOKEN_SUBTRACT, .value.str = NULL, .line = l->line};
  }

  else if (l->ch == '+') {
    lexer_read_char(l);
    return (token){.kind = TOKEN_ADD, .value.str = NULL, .line = l->line};
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

  else if (l->ch == '"') {
    lexer_read_char(l);

    size_t capacity = 16;
    size_t length = 0;
    char *string_value = scu_checked_malloc(capacity);

    if (l->ch == '"') {
      lexer_read_char(l);
      string_value[0] = '\0';
      return (token){
          .kind = TOKEN_STRING, .value.str = string_value, .line = l->line};
    }

    while (l->ch != '"' && l->ch != '\0' && l->ch != EOF) {
      if (length >= capacity - 1) {
        capacity *= 2;
        string_value = scu_checked_realloc(string_value, capacity);
      }

      if (l->ch == '\\') {
        lexer_read_char(l);
        char escaped_char;
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
        case '"':
          escaped_char = '"';
          break;
        case '0':
          escaped_char = '\0';
          break;
        default:
          free(string_value);
          return (token){
              .kind = TOKEN_INVALID, .value.character = l->ch, .line = l->line};
        }
        string_value[length++] = escaped_char;
      } else if (l->ch == '\n') {
        string_value[length++] = '\n';
        l->line++;
      } else {
        string_value[length++] = l->ch;
      }
      lexer_read_char(l);
    }

    if (l->ch != '"') {
      free(string_value);
      return (token){.kind = TOKEN_INVALID, .value.str = NULL, .line = l->line};
    }

    lexer_read_char(l);
    string_value[length] = '\0';

    return (token){
        .kind = TOKEN_STRING, .value.str = string_value, .line = l->line};
  }

  else if (isalnum(l->ch) || l->ch == '_') {
    string_slice slice = {.str = l->buffer + l->pos, .len = 0};

    while (isalnum(l->ch) || l->ch == '_') {
      slice.len += 1;
      lexer_read_char(l);
    }

    char *value = NULL;
    string_slice_to_owned(&slice, &value);

    if (strcmp(value, "goto") == 0) {
      free(value);
      return (token){.kind = TOKEN_GOTO, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "if") == 0) {
      free(value);
      return (token){.kind = TOKEN_IF, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "then") == 0) {
      free(value);
      return (token){.kind = TOKEN_THEN, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "int") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_TYPE_INT, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "char") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_TYPE_CHAR, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "fasm_define") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_FASM_DEFINE, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "fasm") == 0) {
      free(value);
      return (token){.kind = TOKEN_FASM, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "loop") == 0) {
      free(value);
      return (token){.kind = TOKEN_LOOP, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "continue") == 0) {
      free(value);
      return (token){
          .kind = TOKEN_CONTINUE, .value.str = NULL, .line = l->line};
    }

    else if (strcmp(value, "break") == 0) {
      free(value);
      return (token){.kind = TOKEN_BREAK, .value.str = NULL, .line = l->line};
    }

    else {
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

void lexer_tokenize(const char *buffer, size_t buffer_len,
                    dynamic_array *tokens, char *include_dir,
                    unsigned int *errors) {
  lexer lexer;
  lexer_init(&lexer, buffer, buffer_len);

  token tok;
  do {
    tok = lexer_next_token(&lexer);

    if (tok.kind == TOKEN_PDIR_INCLUDE) {
      token incl_str_token = lexer_next_token(&lexer);
      size_t total_len =
          strlen(include_dir) + 1 + strlen(incl_str_token.value.str) + 1;
      char *filepath_to_include = malloc(total_len);
      snprintf(filepath_to_include, total_len, "%s/%s", include_dir,
               incl_str_token.value.str);
      char *incl_buffer = NULL;
      size_t incl_buffer_len =
          scu_read_file(filepath_to_include, &incl_buffer, errors);

      lexer_tokenize(incl_buffer, incl_buffer_len, tokens, include_dir, errors);

      dynamic_array_remove(tokens, tokens->count - 1);
      free(filepath_to_include);
      free(incl_str_token.value.str);
      free(incl_buffer);

      continue;
    }

    if (dynamic_array_append(tokens, &tok) != 0) {
      scu_perror(errors, "Failed to append token to array\n");
      exit(1);
    }
  } while (tok.kind != TOKEN_END);
}

const char *lexer_token_kind_to_str(token_kind kind) {
  switch (kind) {
  case TOKEN_GOTO:
    return "goto";
  case TOKEN_IF:
    return "if";
  case TOKEN_THEN:
    return "then";
  case TOKEN_LABEL:
    return "label";
  case TOKEN_FASM_DEFINE:
    return "fasm_define";
  case TOKEN_FASM:
    return "fasm";
  case TOKEN_LOOP:
    return "loop declare";
  case TOKEN_CONTINUE:
    return "continue";
  case TOKEN_BREAK:
    return "break";
  case TOKEN_PDIR_INCLUDE:
    return "pdir_include";
  case TOKEN_TYPE_INT:
    return "type_int";
  case TOKEN_TYPE_CHAR:
    return "type_char";
  case TOKEN_IDENTIFIER:
    return "identifier";
  case TOKEN_INT:
    return "int";
  case TOKEN_CHAR:
    return "char";
  case TOKEN_STRING:
    return "string";
  case TOKEN_POINTER:
    return "pointer";
  case TOKEN_ASSIGN:
    return "assign";
  case TOKEN_LPAREN:
    return "bracket open";
  case TOKEN_RPAREN:
    return "bracket close";
  case TOKEN_LBRACE:
    return "brace open";
  case TOKEN_RBRACE:
    return "brace close";
  case TOKEN_LSQBR:
    return "square bracket open";
  case TOKEN_RSQBR:
    return "square bracket close";
  case TOKEN_COMMA:
    return "comma";
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

void lexer_print_tokens(dynamic_array *tokens) {
  scu_pdebug("Lexing Debug Statements:\n");

  for (unsigned int i = 0; i < tokens->count; i++) {
    token token;
    dynamic_array_get(tokens, i, &token);

    printf("[line %zu] ", token.line);

    const char *kind = lexer_token_kind_to_str(token.kind);
    printf("%s", kind);

    switch (token.kind) {
    case TOKEN_INT:
      printf("(%d)", token.value.integer);
      break;
    case TOKEN_CHAR:
      printf("(%c)", token.value.character);
      break;
    case TOKEN_STRING:
      printf(" \"%s\"", token.value.str);
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
        token->kind == TOKEN_POINTER || token->kind == TOKEN_STRING) {
      free(token->value.str);
    }
  }
}
