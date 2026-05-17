/*
 * prova: Unit testing framework in C, for C
 *
 * Copyright (c) 2026, Abhigyan Kumar <314abh at gmail dot com>
 * Copyright (c) 2026, Samyak Bambole <bambole@duck.com>
 *
 * Modified from v1.0: add a few macros of my own
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

#ifndef PROVA_H
#define PROVA_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

/* === CONSTANTS START === */

#define PROVA_FAIL_MSG_MAX 512
#define PROVA_ASSERT_ARRAY_BUFFER 1024

#ifndef PROVA_FLOAT_EPSILON
#define PROVA_FLOAT_EPSILON 10e-5
#endif /* PROVA_FLOAT_EPSILON */

/* ===  CONSTANTS END  === */

typedef enum PStatus {
  TEST_FAIL,
  TEST_PASS,
  TEST_SKIP,
  TEST_CRASH,
  TEST_PENDING
} PStatus;

/* Metadata for a unit test scoped to a single function.
 * status: current status of the teest.
 * msg: message to be printed upon failure.
 * name: name of the function to be tested.
 * function: pointer to the test function itself.
 * next: next item in the linked list.
 */
typedef struct PTest {
  PStatus status;
  char *msg;
  const char *name;
  void (*function)(void);
  struct PTest *next;
} PTest;

/* Global metadata on the test suite for final summary. */
typedef struct PMeta {
  unsigned int total_tests;
  unsigned int passing_tests;
  unsigned int failing_tests;
  unsigned int crashing_tests;
  unsigned int skipping_tests;
  float execution_seconds;
  time_t execution_time;
} PMeta;

typedef struct __attribute__((packed)) PAssertCtx {
  uint32_t status;
  uint32_t fail_line;
  char fail_msg[PROVA_FAIL_MSG_MAX];
} PAssertCtx;

extern PMeta p_metadata;
extern PTest *p_registry;
extern thread_local PAssertCtx *p_assert_ctx;

void prova_run_tests(PTest *registry);
size_t prova_count_tests(const PTest *registry);
void prova_print_summary(const PTest *registry);
void prova_cleanup_messages(const PTest *registry);

#define PTEST(f_name) PTEST_MESSAGE(f_name, NULL)
#define PTEST_MESSAGE(f_name, message)                                         \
  void test_function_##f_name(void);                                           \
  __attribute__((constructor)) void register_##f_name(void) {                  \
    static PTest t;                                                            \
    t.status = TEST_PENDING;                                                   \
    t.msg = message;                                                           \
    t.name = #f_name;                                                          \
    t.function = test_function_##f_name;                                       \
    t.next = p_registry;                                                       \
    p_registry = &t;                                                           \
  }                                                                            \
  void test_function_##f_name(void)

/* === COMMON ASSERTION MACROS START === */

/* helper function for assertions. all asserts direct to this to record
 * failures. */
static inline void prova_fail(uint32_t line, const char *file,
                              const char *expr) {
  if (p_assert_ctx == NULL)
    return;
  p_assert_ctx->fail_line = line;
  p_assert_ctx->status = TEST_FAIL;
  snprintf(p_assert_ctx->fail_msg, PROVA_FAIL_MSG_MAX, "%s:%d: %s", file, line,
           expr);
}

#define PROVA_ASSERT(expr)                                                     \
  do {                                                                         \
    if (!(expr)) {                                                             \
      prova_fail(__LINE__, __FILE__, #expr);                                   \
    }                                                                          \
  } while (0)

/* Forced assertions for immediate failures.
 *
 * PROVA_ASSERT_TRUE(expr): fails if @expr is false.
 * PROVA_ASSERT_FALSE(expr): fails if @expr is true.
 * PROVA_ASSERT_NULL(ptr): fails if @ptr is not NULL.
 * PROVA_ASSERT_NOT_NULL(ptr): fails if @ptr is NULL.
 */
#define PROVA_ASSERT_TRUE(expr) PROVA_ASSERT((expr) != false)
#define PROVA_ASSERT_FALSE(expr) PROVA_ASSERT((expr) == false)
#define PROVA_ASSERT_NULL(ptr) PROVA_ASSERT((ptr) == NULL)
#define PROVA_ASSERT_NOT_NULL(ptr) PROVA_ASSERT((ptr) != NULL)

#define PROVA_ASSERT_EQUAL_PTR(expected, actual)                               \
  do {                                                                         \
    if ((void *)(expected) != (void *)(actual)) {                              \
      prova_fail(__LINE__, __FILE__, #actual " == " #expected);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_PTR(expected, actual)                           \
  do {                                                                         \
    if ((void *)(expected) == (void *)(actual)) {                              \
      prova_fail(__LINE__, __FILE__, #actual " != " #expected);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL(expected, actual)                                   \
  do {                                                                         \
    if (expected != actual) {                                                  \
      prova_fail(__LINE__, __FILE__, #actual " == " #expected);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL(expected, actual)                               \
  do {                                                                         \
    if (expected == actual) {                                                  \
      prova_fail(__LINE__, __FILE__, #actual " != " #expected);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_GREATER_THAN(threshold, actual)                           \
  do {                                                                         \
    if (!((actual) > (threshold))) {                                           \
      prova_fail(__LINE__, __FILE__, #actual " > " #threshold);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_GREATER_EQUAL_THAN(threshold, actual)                     \
  do {                                                                         \
    if (!((actual) >= (threshold))) {                                          \
      prova_fail(__LINE__, __FILE__, #actual " >= " #threshold);               \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_LESS_THAN(threshold, actual)                              \
  do {                                                                         \
    if (!((actual) < (threshold))) {                                           \
      prova_fail(__LINE__, __FILE__, #actual " < " #threshold);                \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_LESS_EQUAL_THAN(threshold, actual)                        \
  do {                                                                         \
    if (!((actual) <= (threshold))) {                                          \
      prova_fail(__LINE__, __FILE__, #actual " <= " #threshold);               \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_FLOAT(expected, actual)                             \
  PROVA_ASSERT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_EQUAL_FLOAT_WITHIN(delta, expected, actual)               \
  do {                                                                         \
    float _e = (float)(expected);                                              \
    float _a = (float)(actual);                                                \
    float _diff = fabsf(_a - _e);                                              \
    float _max = fmaxf(fabsf(_e), fabsf(_a));                                  \
    bool _ok =                                                                 \
        (_max == 0.0f) ? (_diff <= (delta)) : (_diff <= (delta) * _max);       \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 #actual " ~= " #expected " (±" #delta ")");                   \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_FLOAT(expected, actual)                         \
  PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(PROVA_FLOAT_EPSILON, (expected), (actual))

#define PROVA_ASSERT_NOT_EQUAL_FLOAT_WITHIN(delta, expected, actual)           \
  do {                                                                         \
    float _e = (float)(expected);                                              \
    float _a = (float)(actual);                                                \
    float _diff = fabsf(_a - _e);                                              \
    float _max = fmaxf(fabsf(_e), fabsf(_a));                                  \
    bool _ok = (_max == 0.0f) ? (_diff > (delta)) : (_diff > (delta) * _max);  \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 #actual " !~= " #expected " (±" #delta ")");                  \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_STRING(expected, actual)                            \
  do {                                                                         \
    const char *_e = (expected);                                               \
    const char *_a = (actual);                                                 \
    bool _ok = (_e == _a) || (_e && _a && strcmp(_e, _a) == 0);                \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 "strcmp(" #actual ", " #expected ") == 0");                   \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_STRING_LENGTH(expected, actual)                     \
  do {                                                                         \
    const char *_e = (expected);                                               \
    const char *_a = (actual);                                                 \
    bool _ok = (strlen(expected) == strlen(actual));                           \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 "strlen(" #actual ")"                                         \
                 " == "                                                        \
                 "strlen(" #expected ")");                                     \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING(expected, actual)                        \
  do {                                                                         \
    const char *_e = (expected);                                               \
    const char *_a = (actual);                                                 \
    bool _ok =                                                                 \
        (_e != _a) && (_e == NULL || _a == NULL || strcmp(_e, _a) != 0);       \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 "strcmp(" #actual ", " #expected ") != 0");                   \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_NOT_EQUAL_STRING_LENGTH(expected, actual)                 \
  do {                                                                         \
    const char *_e = (expected);                                               \
    const char *_a = (actual);                                                 \
    bool _ok = (strlen(expected) != strlen(actual));                           \
    if (!_ok) {                                                                \
      prova_fail(__LINE__, __FILE__,                                           \
                 "strlen(" #actual ")"                                         \
                 " != "                                                        \
                 "strlen(" #expected ")");                                     \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_ARRAYS(expected, actual, n)                         \
  do {                                                                         \
    char _prova_array_assert_buffer[PROVA_ASSERT_ARRAY_BUFFER];                \
    for (size_t i = 0; i < n; ++i) {                                           \
      if (expected[i] != actual[i]) {                                          \
        snprintf(_prova_array_assert_buffer, PROVA_ASSERT_ARRAY_BUFFER,        \
                 #actual "[%zu]"                                               \
                         " == " #expected "[%zu]",                             \
                 i, i);                                                        \
        prova_fail(__LINE__, __FILE__, _prova_array_assert_buffer);            \
      }                                                                        \
    }                                                                          \
  } while (0);

#define PROVA_ASSERT_NOT_EQUAL_ARRAYS(expected, actual, n)                     \
  do {                                                                         \
    char _prova_array_assert_buffer[PROVA_ASSERT_ARRAY_BUFFER];                \
    for (size_t i = 0; i < n; ++i) {                                           \
      if (expected[i] == actual[i]) {                                          \
        snprintf(_prova_array_assert_buffer, PROVA_ASSERT_ARRAY_BUFFER,        \
                 #actual "[%zu]"                                               \
                         " != " #expected "[%zu]",                             \
                 i, i);                                                        \
        prova_fail(__LINE__, __FILE__, _prova_array_assert_buffer);            \
      }                                                                        \
    }                                                                          \
  } while (0)

#define PROVA_ASSERT_EQUAL_MEMORY(expected, actual, n)                         \
  do {                                                                         \
    if (memcmp(expected, actual, n) != 0) {                                    \
      prova_fail(__LINE__, __FILE__,                                           \
                 "#actual"                                                     \
                 " == "                                                        \
                 "#expected");                                                 \
    }                                                                          \
  } while (0);

#define PROVA_ASSERT_NOT_EQUAL_MEMORY(expected, actual, n)                     \
  do {                                                                         \
    if (memcmp(expected, actual, n) == 0) {                                    \
      prova_fail(__LINE__, __FILE__,                                           \
                 "#actual"                                                     \
                 " != "                                                        \
                 "#expected");                                                 \
    }                                                                          \
  } while (0);

/* ===  COMMON ASSERTION MACROS END  === */

#define PROVA_MUTE_STDERR                                                      \
  int _orig_stderr_fd = dup(STDERR_FILENO);                                    \
  do {                                                                         \
    FILE *__err = freopen("/dev/null", "w", stderr);                           \
    (void)__err;                                                               \
  } while (0);

#define PROVA_UNMUTE_STDERR                                                    \
  do {                                                                         \
    if (_orig_stderr_fd != -1) {                                               \
      fflush(stderr);                                                          \
      dup2(_orig_stderr_fd, STDERR_FILENO);                                    \
      close(_orig_stderr_fd);                                                  \
    }                                                                          \
  } while (0);

#endif /* PROVA_H */
