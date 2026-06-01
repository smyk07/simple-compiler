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

#ifndef PROVA_DEFS_H
#define PROVA_DEFS_H

#define PROVA_MAX_CONCURRENT 8
#define PROVA_FAIL_MSG_MAX 1024
#define PROVA_EXPR_LEN_MAX 1024
#define PROVA_FILENAME_MAX 1024
#define PROVA_ASSERT_ARRAY_BUFFER 1024

#ifndef PROVA_FLOAT_EPSILON
#define PROVA_FLOAT_EPSILON 10e-5
#endif /* PROVA_FLOAT_EPSILON */

#include <stddef.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>

extern struct ProvaTest *PROVA_TEST_QUEUE;
extern struct ProvaTest *PROVA_CURRENT_TEST;

#define STBDS_NO_SHORT_NAMES
#include "tests/stb_ds.h"

typedef enum ProvaStatus {
  TEST_PENDING,
  TEST_PASS,
  TEST_FAIL,
  TEST_SKIP,
  TEST_CRASH,
} ProvaStatus;

typedef struct __attribute__((packed)) ProvaAssertion {
  uint32_t line;
  uint32_t status;
  char expr[PROVA_EXPR_LEN_MAX];
  char filename[PROVA_FILENAME_MAX];
} ProvaAssertion;

typedef struct ProvaTest {
  ProvaStatus status;
  size_t asserts_total;
  size_t asserts_passed;
  size_t asserts_failed;
  size_t asserts_crashed;
  ProvaAssertion *asserts;

  /* user-provided fields */
  char *test_message;
  char *test_name;
  void (*test_method)(void);

  /* for child process operations populated by job runner */
  int readfd;
  pid_t child_pid;
  float exec_duration_ms;
} ProvaTest;

/* === ANSI COLOR SEQUENCES START === */

#define PROVA_ANSI_RESET "\x1b[0m"
#define PROVA_ANSI_BOLD "\x1b[1m"

#define PROVA_ANSI_RED "\x1b[31m"
#define PROVA_ANSI_GREEN "\x1b[32m"
#define PROVA_ANSI_YELLOW "\x1b[33m"
#define PROVA_ANSI_CYAN "\x1b[36m"
#define PROVA_ANSI_MAGENTA "\x1b[35m"

#ifdef PROVA_NO_COLORS

#define PROVA_PASSED_TAG "[PASSED] "
#define PROVA_FAILED_TAG "[FAILED] "
#define PROVA_CRASHED_TAG "[CRASHED]"
#define PROVA_PENDING_TAG "[PENDING]"
#define PROVA_SKIPPED_TAG "[SKIPPED]"

#else

#define PROVA_PASSED_TAG                                                       \
  PROVA_ANSI_BOLD PROVA_ANSI_GREEN "[PASSED] " PROVA_ANSI_RESET
#define PROVA_FAILED_TAG                                                       \
  PROVA_ANSI_BOLD PROVA_ANSI_RED "[FAILED] " PROVA_ANSI_RESET
#define PROVA_CRASHED_TAG                                                      \
  PROVA_ANSI_BOLD PROVA_ANSI_MAGENTA "[CRASHED]" PROVA_ANSI_RESET
#define PROVA_PENDING_TAG                                                      \
  PROVA_ANSI_BOLD PROVA_ANSI_YELLOW "[PENDING]" PROVA_ANSI_RESET
#define PROVA_SKIPPED_TAG                                                      \
  PROVA_ANSI_BOLD PROVA_ANSI_CYAN "[SKIPPED]" PROVA_ANSI_RESET

#endif /* PROVA_NO_COLORS */

/* ===  ANSI COLOR SEQUENCES END  === */

#endif /* PROVA_DEFS_H */
