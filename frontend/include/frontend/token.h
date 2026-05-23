/*
 * token: definitions of lexical tokens
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef TOKEN_H
#define TOKEN_H

#include "core/common.h"
#include "core/ds/dynamic_array.h"

/*
 * @enum token_kind: enumeration of all kinds of tokens supported by the lexer.
 */
typedef enum token_kind {
  TOKEN_INVALID = 0,

  /*
   * Keywords
   */
  TOKEN_GOTO,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_THEN,
  TOKEN_MATCH,
  TOKEN_LOOP,
  TOKEN_WHILE,
  TOKEN_DO_WHILE,
  TOKEN_IN,
  TOKEN_FOR,
  TOKEN_CONTINUE,
  TOKEN_BREAK,
  TOKEN_FN,
  TOKEN_RETURN,

  /*
   * Type specifiers
   */
  TOKEN_TYPE_U8,
  TOKEN_TYPE_U16,
  TOKEN_TYPE_U32,
  TOKEN_TYPE_U64,
  TOKEN_TYPE_U128,

  TOKEN_TYPE_I8,
  TOKEN_TYPE_I16,
  TOKEN_TYPE_I32,
  TOKEN_TYPE_I64,
  TOKEN_TYPE_I128,

  TOKEN_TYPE_BOOL,

  TOKEN_TYPE_CHAR,

  /*
   * Preprocessor Directives
   */
  TOKEN_PDIR_INCLUDE,

  /*
   * Literals
   */
  TOKEN_INT_LITERAL,
  TOKEN_BOOL_LITERAL,
  TOKEN_CHAR_LITERAL,
  TOKEN_STRING_LITERAL,

  /*
   * Identifiers
   */
  TOKEN_IDENTIFIER,
  TOKEN_LABEL,
  TOKEN_POINTER,    // *identifier
  TOKEN_ADDRESS_OF, // &identifier

  /*
   * Delimiters
   */
  TOKEN_LPAREN, // (
  TOKEN_RPAREN, // )
  TOKEN_LBRACE, // {
  TOKEN_RBRACE, // }
  TOKEN_LSQBR,  // [
  TOKEN_RSQBR,  // ]
  TOKEN_COMMA,  // ,
  TOKEN_COLON,  // :

  /*
   * Operators - Arithmetic
   */
  TOKEN_ASSIGN,   // =
  TOKEN_ADD,      // +
  TOKEN_SUBTRACT, // -
  TOKEN_MULTIPLY, // *
  TOKEN_DIVIDE,   // /
  TOKEN_MODULO,   // %

  /*
   * Operators - Relational
   */
  TOKEN_IS_EQUAL,              // ==
  TOKEN_NOT_EQUAL,             // !=
  TOKEN_LESS_THAN,             // <
  TOKEN_LESS_THAN_OR_EQUAL,    // <=
  TOKEN_GREATER_THAN,          // >
  TOKEN_GREATER_THAN_OR_EQUAL, // >=

  /*
   * Operators - Logical
   */
  TOKEN_NOT, // !
  TOKEN_AND, // &&
  TOKEN_OR,  // ||

  /*
   * Special Symbols
   */
  TOKEN_DARROW,     // =>
  TOKEN_UNDERSCORE, // _
  TOKEN_ELLIPSIS,   // ...

  /*
   * Special Tokens
   */
  TOKEN_END
} token_kind;

typedef enum token_literal_value_kind {
  TLV_NULL = 0,

  TLV_INT,
  TLV_CHAR,
  TLV_BOOL,
  TLV_STR,
} token_literal_value_kind;

/*
 * @struct token_literal_value: holds the "value" of any particular literal
 * token. Values can be integers, characters, or strings for labels.
 */
typedef struct token_literal_value {
  token_literal_value_kind kind;

  union {
    u32 integer;
    char character;
    bool boolean;
    char *str;
  };
} token_literal_value;

/*
 * @struct token: reprents a token and its metadata.
 */
typedef struct token {
  token_kind kind;
  token_literal_value value;
  u64 line; // <-- Where the token is placed in the source buffer.
} token;

/*
 * @brief: Converts a token_kind enum value to its string representation.
 *
 * @param kind: token_kind enum value.
 * @return: string representation of the token.
 */
const char *token_kind_to_str(token_kind kind);

/*
 * @brief: Converts a token_literal_value  to its string representation.
 *
 * @param token: token value to extract.
 * @return: string representation of the token value.
 */
char *token_get_value(token token);

/*
 * @brief: Print the whole token stream, required for debugging.
 *
 * @param tokens: dynamic_array of tokens.
 */
void token_print_tokens(dynamic_array *tokens);

/*
 * @brief: Free / Destroy tokens before termination. This function is needed
 * due to there being malloc'd strings in token->value.
 *
 * @param tokens: dynamic_array of tokens.
 */
void free_tokens(dynamic_array *tokens);

#endif // !TOKEN_H
