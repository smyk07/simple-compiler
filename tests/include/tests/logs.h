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

/* This file was generated with the assistance of the following LLMs:
 *   - Claude Haiku 4.5, via GitHub Copilot
 *   - Big Pickle, via OpenCode Zen
 *
 * Reviewed by @314abh */

#ifndef PROVA_LOGS_H
#define PROVA_LOGS_H

#include <stddef.h>
#include <stdio.h>

/* Asynchronous logging system for Prova test framework.
 *
 * Usage:
 *   prova_log_init(stdout);           // initialize with stdout
 *   prova_log("Test %s passed", name);
 *   prova_log_fini();                 // wait for pending logs, shutdown
 */

typedef struct {
  FILE *dst;  /* destination: stdout, stderr, or file handle */
  int active; /* logger thread running flag */
} ProvaLog;

extern ProvaLog prova_log_ctx;

/* Initialize logger with output destination (FILE*).
 * Spawns dedicated logger thread.
 * safe_to_call: multiple times (only first init takes effect).
 */
void prova_log_init(FILE *out);

/* Enqueue formatted log message (non-blocking).
 * Format: printf-style format string + args.
 * Fails silently if buffer full (oldest log dropped).
 */
void prova_log(const char *fmt, ...);

/* Log test result (convenience function).
 * Formats: "[TEST_NAME] status msg" or "[TEST_NAME] status: msg (N
 * assertions)".
 */
void prova_log_test(const char *name, int passed, int total, const char *msg);

/* Log assertion result (convenience function).
 * Formats: "[ASSERTION] file:line: expr (PASS|FAIL)".
 */
void prova_log_assert(const char *file, int line, const char *expr, int ok);

/* Flush pending logs and shutdown logger thread.
 * Blocks until all queued logs written.
 * safe_to_call: multiple times (idempotent).
 */
void prova_log_fini(void);

#endif /* PROVA_LOGS_H */
