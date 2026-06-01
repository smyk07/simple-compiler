#include "tests/prova.h"

#include "core/ds/dynamic_array.h"
#include "core/utils.h"

#include "frontend/lexer.h"
#include "frontend/token.h"

PTEST(lexer_empty_str) {
  dynamic_array arr;
  dynamic_array_init(&arr, sizeof(token));

  scu_result res = lexer_tokenize("", 0, &arr, "");

  PROVA_ASSERT_EQUAL(SCU_SUCCESS, res);
  PROVA_ASSERT_EQUAL(1, arr.count);
  PROVA_ASSERT_EQUAL(TOKEN_END, *(token_kind *)arr.items);

  dynamic_array_free(&arr);
}

PTEST(lexer_whitespace) {
  dynamic_array arr;
  dynamic_array_init(&arr, sizeof(token));

  scu_result res = lexer_tokenize("\t         ", 10, &arr, "");

  PROVA_ASSERT_EQUAL(SCU_SUCCESS, res);
  PROVA_ASSERT_EQUAL(1, arr.count);
  PROVA_ASSERT_EQUAL(TOKEN_END, ((token_kind *)arr.items)[0]);

  dynamic_array_free(&arr);
}

PTEST(lexer_invalid) {
  dynamic_array arr;
  dynamic_array_init(&arr, sizeof(token));

  PROVA_MUTE_STDERR
  scu_result res = lexer_tokenize(";", 1, &arr, "");
  PROVA_UNMUTE_STDERR

  PROVA_ASSERT_EQUAL(SCU_ERR_LEX, res);

  dynamic_array_free(&arr);
}

PTEST(lexer_stream_test) {
  dynamic_array arr;
  dynamic_array_init(&arr, sizeof(token));

  scu_result res =
      lexer_tokenize("Hello World, i8 i16 true false while", 36, &arr, "");

  PROVA_ASSERT_EQUAL(SCU_SUCCESS, res);
  PROVA_ASSERT_EQUAL(9, arr.count);

  token *tok = dynamic_array_get_ptr(&arr, 0);
  PROVA_ASSERT_EQUAL(TOKEN_IDENTIFIER, tok->kind);
  PROVA_ASSERT_EQUAL(TLV_STR, tok->value.kind);
  PROVA_ASSERT_EQUAL_STRING("Hello", tok->value.str);

  tok = dynamic_array_get_ptr(&arr, 1);
  PROVA_ASSERT_EQUAL(TOKEN_IDENTIFIER, tok->kind);
  PROVA_ASSERT_EQUAL(TLV_STR, tok->value.kind);
  PROVA_ASSERT_EQUAL_STRING("World", tok->value.str);

  tok = dynamic_array_get_ptr(&arr, 2);
  PROVA_ASSERT_EQUAL(TOKEN_COMMA, tok->kind);

  tok = dynamic_array_get_ptr(&arr, 3);
  PROVA_ASSERT_EQUAL(TOKEN_TYPE_I8, tok->kind);

  tok = dynamic_array_get_ptr(&arr, 4);
  PROVA_ASSERT_EQUAL(TOKEN_TYPE_I16, tok->kind);

  tok = dynamic_array_get_ptr(&arr, 5);
  PROVA_ASSERT_EQUAL(TOKEN_BOOL_LITERAL, tok->kind);
  PROVA_ASSERT_EQUAL(TLV_BOOL, tok->value.kind);
  PROVA_ASSERT_EQUAL(true, tok->value.boolean);

  tok = dynamic_array_get_ptr(&arr, 6);
  PROVA_ASSERT_EQUAL(TOKEN_BOOL_LITERAL, tok->kind);
  PROVA_ASSERT_EQUAL(TLV_BOOL, tok->value.kind);
  PROVA_ASSERT_EQUAL(false, tok->value.boolean);

  tok = dynamic_array_get_ptr(&arr, 7);
  PROVA_ASSERT_EQUAL(TOKEN_WHILE, tok->kind);

  tok = dynamic_array_get_ptr(&arr, 8);
  PROVA_ASSERT_EQUAL(TOKEN_END, tok->kind);

  dynamic_array_free(&arr);
}
