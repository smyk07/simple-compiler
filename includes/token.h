/*
 * token: Definitions of lexical token.
 */

#ifndef TOKEN
#define TOKEN

/*
 * @enum token_kind: enumeration of all kinds of tokens supported by the lexer.
 */
#include <stddef.h>
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

#endif // !TOKEN
