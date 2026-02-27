/*
 * token: definitions of lexical tokens
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "token.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

const char *lexer_token_kind_to_str(token_kind kind) {
  switch (kind) {
  case TOKEN_GOTO:
    return "goto";
  case TOKEN_IF:
    return "if";
  case TOKEN_ELSE:
    return "else";
  case TOKEN_THEN:
    return "then";
  case TOKEN_MATCH:
    return "match";
  case TOKEN_LOOP:
    return "loop declare";
  case TOKEN_WHILE:
    return "while loop declare";
  case TOKEN_DO_WHILE:
    return "do-while loop declare";
  case TOKEN_IN:
    return "in";
  case TOKEN_FOR:
    return "for loop declare";

  case TOKEN_CONTINUE:
    return "continue";
  case TOKEN_BREAK:
    return "break";
  case TOKEN_FN:
    return "fn (signature begin)";
  case TOKEN_RETURN:
    return "return";

  case TOKEN_TYPE_INT:
    return "type_int";
  case TOKEN_TYPE_CHAR:
    return "type_char";

  case TOKEN_PDIR_INCLUDE:
    return "pdir_include";

  case TOKEN_INT_LITERAL:
    return "int";
  case TOKEN_CHAR_LITERAL:
    return "char";
  case TOKEN_STRING_LITERAL:
    return "string";

  case TOKEN_IDENTIFIER:
    return "identifier";
  case TOKEN_LABEL:
    return "label";
  case TOKEN_POINTER:
    return "pointer";
  case TOKEN_ADDRESS_OF:
    return "addof";

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
  case TOKEN_COLON:
    return "colon";

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

  case TOKEN_DARROW:
    return "=> (darrow)";
  case TOKEN_UNDERSCORE:
    return "_ (underscore)";
  case TOKEN_ELLIPSIS:
    return "... (ellipsis)";

  case TOKEN_INVALID:
    return "invalid";
  case TOKEN_END:
    return "end";
  }
}

void lexer_print_tokens(dynamic_array *tokens) {
  for (u64 i = 0; i < tokens->count; i++) {
    token token;
    dynamic_array_get(tokens, i, &token);

    printf("[line %" PRIu64 "] ", token.line);

    const char *kind = lexer_token_kind_to_str(token.kind);
    printf("%s", kind);

    switch (token.kind) {
    case TOKEN_INT_LITERAL:
      printf("(%d)", token.value.integer);
      break;
    case TOKEN_CHAR_LITERAL:
      printf("(%c)", token.value.character);
      break;
    case TOKEN_STRING_LITERAL:
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
  for (u64 i = 0; i < tokens->count; i++) {
    token *token = tokens->items + (i * tokens->item_size);
    if (token->kind == TOKEN_IDENTIFIER || token->kind == TOKEN_LABEL ||
        token->kind == TOKEN_INVALID || token->kind == TOKEN_ADDRESS_OF ||
        token->kind == TOKEN_POINTER || token->kind == TOKEN_STRING_LITERAL) {
      free(token->value.str);
    }
  }
}
