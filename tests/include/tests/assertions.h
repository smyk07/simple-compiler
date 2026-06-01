/* Copyright 2026 Abhigyan Kumar <314abh at gmail dot com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROVA_ASSERTIONS_H
#define PROVA_ASSERTIONS_H

#include "tests/defs.h"
#include "tests/logs.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static inline void prova_record_assertion(size_t line, const char *filename,
                                          const char *expr, bool passed) {
  if (PROVA_CURRENT_TEST == NULL)
    return;

  // prepare the assertion
  ProvaAssertion assertion = {0};
  assertion.line = line;
  assertion.status = passed ? TEST_PASS : TEST_FAIL;
  strncpy(assertion.expr, expr, PROVA_EXPR_LEN_MAX);
  strncpy(assertion.filename, filename, PROVA_FILENAME_MAX);

  // append the assertion to the assertions array in test case being
  // executed in the current thread. PROVA_CURRENT_TEST is a thread
  // local global state.
  ProvaTest *current_test = PROVA_CURRENT_TEST;
  stbds_arrput(current_test->asserts, assertion);

  // Log the assertion
  prova_log_assert(filename, line, expr, passed);
}

/* === common ASSERTION MACROS START === */

#define PROVA_ASSERT(expr)                                                     \
  do {                                                                         \
    bool _prova_ok = !!(expr);                                                 \
    prova_record_assertion(__LINE__, __FILE__, #expr, _prova_ok);              \
  } while (0)

#define PROVA_ASSERT_TRUE(expr) PROVA_ASSERT((expr) != false)
#define PROVA_ASSERT_FALSE(expr) PROVA_ASSERT((expr) == false)
#define PROVA_ASSERT_NULL(ptr) PROVA_ASSERT((ptr) == NULL)
#define PROVA_ASSERT_NOT_NULL(ptr) PROVA_ASSERT((ptr) != NULL)

#define PROVA_ASSERT_EQUAL_PTR(expected, actual)                               \
  do {                                                                         \
    bool _prova_ok = (void *)(expected) == (void *)(actual);                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " == " #expected,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_PTR(expected, actual)                           \
  do {                                                                         \
    bool _prova_ok = (void *)(expected) != (void *)(actual);                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " != " #expected,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_EQUAL(expected, actual)                                   \
  do {                                                                         \
    bool _prova_ok = (expected) == (actual);                                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " == " #expected,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL(expected, actual)                               \
  do {                                                                         \
    bool _prova_ok = (expected) != (actual);                                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " != " #expected,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_GREATER_THAN(threshold, actual)                           \
  do {                                                                         \
    bool _prova_ok = (actual) > (threshold);                                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " > " #threshold,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_GREATER_EQUAL_THAN(threshold, actual)                     \
  do {                                                                         \
    bool _prova_ok = (actual) >= (threshold);                                  \
    prova_record_assertion(__LINE__, __FILE__, #actual " >= " #threshold,      \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_LESS_THAN(threshold, actual)                              \
  do {                                                                         \
    bool _prova_ok = (actual) < (threshold);                                   \
    prova_record_assertion(__LINE__, __FILE__, #actual " < " #threshold,       \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_LESS_EQUAL_THAN(threshold, actual)                        \
  do {                                                                         \
    bool _prova_ok = (actual) <= (threshold);                                  \
    prova_record_assertion(__LINE__, __FILE__, #actual " <= " #threshold,      \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_EQUAL_FLOAT(expected, actual)                             \
  PROVA_ASSERT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_EQUAL_FLOAT_WITHIN(delta, expected, actual)               \
  do {                                                                         \
    float _e = (float)(expected);                                              \
    float _a = (float)(actual);                                                \
    float _diff = fabsf(_a - _e);                                              \
    float _max = fmaxf(fabsf(_e), fabsf(_a));                                  \
    bool _prova_ok =                                                           \
        (_max == 0.0f) ? (_diff <= (delta)) : (_diff <= (delta) * _max);       \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           #actual " ~= " #expected " (\xC2\xB1" #delta ")",   \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_FLOAT(expected, actual)                         \
  PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(delta, expected, actual)           \
  do {                                                                         \
    float _e = (float)(expected);                                              \
    float _a = (float)(actual);                                                \
    float _diff = fabsf(_a - _e);                                              \
    float _max = fmaxf(fabsf(_e), fabsf(_a));                                  \
    bool _prova_ok =                                                           \
        (_max == 0.0f) ? (_diff > (delta)) : (_diff > (delta) * _max);         \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           #actual " !~= " #expected " (\xC2\xB1" #delta ")",  \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_EQUAL_STRING(expected, actual)                            \
  do {                                                                         \
    const char *_prova_e = (expected);                                         \
    const char *_prova_a = (actual);                                           \
    bool _prova_ok =                                                           \
        (_prova_e == _prova_a) ||                                              \
        (_prova_e && _prova_a && strcmp(_prova_e, _prova_a) == 0);             \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "strcmp(" #actual ", " #expected ") == 0",          \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_EQUAL_STRING_LENGTH(expected, actual)                     \
  do {                                                                         \
    bool _prova_ok = strlen(expected) == strlen(actual);                       \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "strlen(" #actual ") == strlen(" #expected ")",     \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING(expected, actual)                        \
  do {                                                                         \
    const char *_prova_e = (expected);                                         \
    const char *_prova_a = (actual);                                           \
    bool _prova_ok =                                                           \
        (_prova_e != _prova_a) && (_prova_e == NULL || _prova_a == NULL ||     \
                                   strcmp(_prova_e, _prova_a) != 0);           \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "strcmp(" #actual ", " #expected ") != 0",          \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING_LENGTH(expected, actual)                 \
  do {                                                                         \
    bool _prova_ok = strlen(expected) != strlen(actual);                       \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "strlen(" #actual ") != strlen(" #expected ")",     \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_EQUAL_ARRAYS(expected, actual, n)                         \
  do {                                                                         \
    char _prova_buf[PROVA_ASSERT_ARRAY_BUFFER];                                \
    for (size_t i = 0; i < n; ++i) {                                           \
      bool _prova_ok = (expected)[i] == (actual)[i];                           \
      snprintf(_prova_buf, PROVA_ASSERT_ARRAY_BUFFER,                          \
               #actual "[%zu] == " #expected "[%zu]", i, i);                   \
      prova_record_assertion(__LINE__, __FILE__, _prova_buf, _prova_ok);       \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_ARRAYS(expected, actual, n)                     \
  do {                                                                         \
    char _prova_buf[PROVA_ASSERT_ARRAY_BUFFER];                                \
    for (size_t i = 0; i < n; ++i) {                                           \
      bool _prova_ok = (expected)[i] != (actual)[i];                           \
      snprintf(_prova_buf, PROVA_ASSERT_ARRAY_BUFFER,                          \
               #actual "[%zu] != " #expected "[%zu]", i, i);                   \
      prova_record_assertion(__LINE__, __FILE__, _prova_buf, _prova_ok);       \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_MEMORY(expected, actual, n)                         \
  do {                                                                         \
    bool _prova_ok = memcmp(expected, actual, n) == 0;                         \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "memcmp(" #actual ", " #expected ", " #n ") == 0",  \
                           _prova_ok);                                         \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_MEMORY(expected, actual, n)                     \
  do {                                                                         \
    bool _prova_ok = memcmp(expected, actual, n) != 0;                         \
    prova_record_assertion(__LINE__, __FILE__,                                 \
                           "memcmp(" #actual ", " #expected ", " #n ") != 0",  \
                           _prova_ok);                                         \
  } while (0)

/* ===  COMMON ASSERTION MACROS END  === */

#endif /* PROVA_ASSERTIONS_H */
