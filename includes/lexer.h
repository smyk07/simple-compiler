/**
 * lexer: Lexical analyzer for the simple-compiler
 */

#ifndef LEXER_H
#define LEXER_H

#include "ds/dynamic_array.h"

#include <stddef.h>

/*
 * @enum token_kind: enumeration of all the tokens supported by the lexer.
 */
typedef enum token_kind {
  /*
   * Keywords
   *
   * INPUT and OUTPUT will obviously be temporary until I implement an FFI or a
   * standard library.
   */
  TOKEN_INPUT = 0,
  TOKEN_OUTPUT,
  TOKEN_GOTO,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_LABEL,
  TOKEN_TYPE_INT,
  TOKEN_TYPE_CHAR,
  TOKEN_POINTER,

  /*
   * Literals
   */
  TOKEN_IDENTIFIER,
  TOKEN_INT,
  TOKEN_CHAR,

  /*
   * Brackets
   */
  TOKEN_BRACKET_OPEN,
  TOKEN_BRACKET_CLOSE,

  /*
   * Arithmetic Operators
   */
  TOKEN_ASSIGN,
  TOKEN_ADD,
  TOKEN_SUBTRACT,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_MODULO,
  TOKEN_ADDRESS_OF,

  /*
   * Conditional Operators
   */
  TOKEN_IS_EQUAL,
  TOKEN_NOT_EQUAL,
  TOKEN_LESS_THAN,
  TOKEN_LESS_THAN_OR_EQUAL,
  TOKEN_GREATER_THAN,
  TOKEN_GREATER_THAN_OR_EQUAL,

  /*
   * Special Tokens
   */
  TOKEN_INVALID,
  TOKEN_COMMENT,
  TOKEN_END
} token_kind;

/*
 * @union token_value: holds the "value" of any particular token. Values can be
 * integers, characters, or strings for labels.
 */
typedef union token_value {
  int integer;
  char character;
  char *str;
} token_value;

/*
 * @struct token: reprents a token and its metadata.
 */
typedef struct token {
  token_kind kind;
  token_value value;
  size_t line; // <-- Where the token is placed in the source buffer.
} token;

/*
 * @struct lexer: maintains state of the lexer for tokenizing the source buffer.
 */
typedef struct lexer {
  /*
   * Pointer to source buffer and its size in bytes.
   */
  const char *buffer;
  size_t buffer_len;

  /*
   * Data concerned with current lexer state.
   */
  size_t line;     // <-- tracks the current line
  size_t pos;      // <-- current position in buffer
  size_t read_pos; // <-- next read position (usually pos + 1)
  char ch;         // <-- character at buffer[read_pos]
} lexer;

/*
 * @brief: Tokenize a string buffer into a dynamic_array of tokens.
 *
 * @param buffer: string to be tokenized.
 * @param buffer_len size of buffer (in bytes).
 * @param tokens: dynamic_array of tokens (should be initialized).
 * @param errors: error counter to increment whenever an errror is encountered.
 */
void lexer_tokenize(const char *buffer, size_t buffer_len,
                    dynamic_array *tokens, unsigned int *errors);

/*
 * @brief: Converts a token_kind enum value to its string representation.
 *
 * @param kind: token_kind enum value.
 * @return: string representation of the token.
 */
const char *lexer_token_kind_to_str(token_kind kind);

/*
 * @brief: Print the whole token stream, required for debugging.
 *
 * @param tokens: dynamic_array of tokens.
 */
void lexer_print_tokens(dynamic_array *tokens);

/*
 * @brief: Free / Destroy tokens before termination. This function is needed due
 * to there being malloc'd strings in token->value.
 *
 * @param tokens: dynamic_array of tokens.
 */
void free_tokens(dynamic_array *tokens);

#endif // !LEXER_H
