/*
 * token: Definitions of lexical token.
 */

#ifndef TOKEN
#define TOKEN

#include <stddef.h>

/*
 * @enum token_kind: enumeration of all kinds of tokens supported by the lexer.
 */
typedef enum token_kind {
  /*
   * Keywords
   */
  TOKEN_GOTO = 0,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_TYPE_INT,
  TOKEN_TYPE_CHAR,
  TOKEN_FASM_DEFINE, // fasm definitions, goes before _start
  TOKEN_FASM,        // inline fasm copy and paste
  TOKEN_LOOP,
  TOKEN_CONTINUE,
  TOKEN_BREAK,

  /*
   * Preprocessor Directives
   */
  TOKEN_PDIR_INCLUDE,

  /*
   * Literals
   */
  TOKEN_IDENTIFIER,
  TOKEN_LABEL,
  TOKEN_INT,
  TOKEN_CHAR,
  TOKEN_STRING,
  TOKEN_POINTER,

  /*
   * Brackets
   */
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LSQBR,
  TOKEN_RSQBR,

  /*
   * Delimiters
   */
  TOKEN_COMMA,

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
