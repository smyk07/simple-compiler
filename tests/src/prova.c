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


#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#define STB_DS_IMPLEMENTATION
#include "tests/prova.h"
#include "tests/logs.h"

#define STB_SPRINTF_DECORATE(name) stb_##name
#include "tests/stb_sprintf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// Once the registry of tests is prepared, we can launch test cases as
// separate child processes. That's what prova_launch_tests does.

ProvaTest *PROVA_TEST_QUEUE = NULL;
ProvaTest *PROVA_CURRENT_TEST = NULL;

/* for inter-process test data sharing */
static ssize_t prova_read(int fd, const void *buff, size_t count);
static ssize_t prova_write(int fd, const void *buff, size_t count);

static void prova_launch_tests(void);
static void prova_log_result(ProvaTest *test);
static void prova_exec_test(ProvaTest *test);
static void prova_collect_test(ProvaTest *test, int wstatus);

static inline const char *prova_status_to_str(ProvaTest *test);

static ssize_t prova_read(int fd, const void *buff, size_t count) {
    if (count <= 0 || buff == NULL) return 0;

    size_t bytes_read = 0;
    while (bytes_read < count) {
        ssize_t ret = read(fd, (char*) buff + bytes_read, count - bytes_read);
        if (ret <= 0) {
            perror("prova_read: couldn't read data from child. exiting ...\n");
            return -1;
        }
        bytes_read += ret;
    }

    return bytes_read;
}

static ssize_t prova_write(int fd, const void *buff, size_t count) {
    if (count <= 0 || buff == NULL) return 0;

    size_t bytes_written = 0;
    while (bytes_written < count) {
        ssize_t ret = write(fd, (char*) buff + bytes_written, count - bytes_written);
        if (ret <= 0) {
            perror("prova_write: couldn't write data to parent. exiting ...\n");
            return -1;
        }
        bytes_written += ret;
    }

    return bytes_written;
}

static inline const char *prova_status_to_str(ProvaTest *test) {
  static const char untagged[] = "[UNTAGGED]";

  switch (test->status) {
  case TEST_PENDING:
    return PROVA_PENDING_TAG;
  case TEST_PASS:
    return PROVA_PASSED_TAG;
  case TEST_FAIL:
    return PROVA_FAILED_TAG;
  case TEST_SKIP:
    return PROVA_SKIPPED_TAG;
  case TEST_CRASH:
    return PROVA_CRASHED_TAG;
  default:
    return untagged;
  }

  return untagged;
}

static void prova_log_result(ProvaTest *test) {
  const char *tag = prova_status_to_str(test);
  size_t nfail = 0, ntotal = stbds_arrlenu(test->asserts);

  if (test->status == TEST_FAIL) {
    for (size_t i = 0; i < ntotal; ++i)
      if (test->asserts[i].status == TEST_FAIL)
        nfail++;
  }

  /* Build one big string and log it once */
  if (test->status == TEST_FAIL && nfail > 0) {
    /* Headline + each failing expr: "  [FAILED] name : file\n    expr\n expr\n"
     */
    size_t sz = PROVA_FAIL_MSG_MAX + PROVA_FILENAME_MAX +
                nfail * (PROVA_EXPR_LEN_MAX + 4);
    char *buf = (char *)malloc(sz);
    if (!buf)
      return;

    size_t pos = stb_snprintf(buf, sz, "  %s %s : %s\n", tag, test->test_name,
                              test->asserts[0].filename);
    for (size_t i = 0; i < ntotal; ++i) {
      ProvaAssertion *a = &test->asserts[i];
      if (a->status != TEST_FAIL)
        continue;
      pos += stb_snprintf(buf + pos, sz - pos, "    %s\n", a->expr);
    }

    buf[pos] = '\0';
    prova_log("%s", buf);
    free(buf);
  } else {
    prova_log("  %s %s\n", tag, test->test_name);
  }
}

static void prova_exec_test(ProvaTest *test) {
  int pipefd[2];
  pipe(pipefd);
  pid_t pid = fork();
  int readfd = pipefd[0], writefd = pipefd[1];

  // TODO: handle inability to create child process
  //
  // If the program cannot create child processes, the program should
  // execute tests in a linar fashion and log an INFO message to
  // stdout or the required filestream alerting that programs will not
  // be run in parallel, bypassing default behavior.
  assert(pid != -1 && "prova_exec_test: couldn't create child proces.");

  if (pid == 0) { // inside child process
    close(readfd);

    PROVA_CURRENT_TEST = test;
    test->test_method(); /* test->asserts has been populated */
    size_t count = stbds_arrlen(test->asserts);
    prova_write(writefd, &count, sizeof(count));
    prova_write(writefd, test->asserts, count * sizeof(*test->asserts));

    close(writefd);
    _exit(0);
  }

  close(writefd);
  test->child_pid = pid;
  test->readfd = readfd;
}

static void prova_collect_test(ProvaTest *test, int wstatus) {
  size_t count = 0;
  int readfd = test->readfd;

  if (WIFSIGNALED(wstatus)) {
    /* Child crashed — do not attempt to read assertions from pipe. */
    test->status = TEST_CRASH;

    /* Cleanup pipe state */
    if (readfd >= 0)
      close(readfd);
    test->readfd = -1;
    test->child_pid = 0;
    prova_log_result(test);
    return;
  }

  /* Child exited normally — read assertions */
  if (readfd >= 0) {
    if (prova_read(readfd, &count, sizeof(count)) > 0) {
      stbds_arrsetlen(test->asserts, count);
      prova_read(readfd, test->asserts, count * sizeof(*test->asserts));
    } else {
      /* No data written by child — treat as zero assertions */
      stbds_arrsetlen(test->asserts, 0);
      count = 0;
    }
    close(readfd);
  }

  test->readfd = -1;
  test->child_pid = 0;

  /* Determine pass/fail based on assertions */
  test->status = TEST_PASS;
  for (size_t i = 0; i < count; ++i) {
    if (test->asserts[i].status == TEST_FAIL) {
      test->status = TEST_FAIL;
      break;
    }
  }

  prova_log_result(test);
}

/* Launch tests ensuring that at any given time no more than
 * PROVA_MAX_CONCURRENT tests are running in parallel.
 *
 * We launch tests and collect tests in a common loop. The loop keeps
 * running until we've exhausted our test queue. Within the loop, we
 * fist check if the number of tests running at the time are less than
 * PROVA_MAX_CONCURRENT. If it is, we launch more tests until we reach
 * that number. */
static void prova_launch_tests(void) {
  size_t launched = 0, running = 0;
  size_t test_queue_length = stbds_arrlenu(PROVA_TEST_QUEUE);

  ProvaTest **running_tests = NULL;

  while (launched < test_queue_length || running) {
    /* fill available slots */
    while (launched < test_queue_length &&
           stbds_arrlenu(running_tests) < PROVA_MAX_CONCURRENT) {
      ProvaTest *ct = &PROVA_TEST_QUEUE[launched];
      if (ct->status == TEST_SKIP)
        prova_log_result(ct);
      prova_exec_test(ct);
      stbds_arrput(running_tests, ct);
      launched++;
      running++;
    }

    if (running == 0)
      continue;
    int wstatus;
    pid_t pid = waitpid(-1, &wstatus, 0);
    assert(pid > 0 && "prova_launch_tests: couldn't collect child process.");

    /* identify an exiting test */
    for (size_t i = 0; i < stbds_arrlenu(running_tests); ++i) {
      /* TODO: replace linear search with hash table
       *
       * Linear search is fine for small concurrent operations but
       * there can be hundreds of concurrent processes. In that case,
       * a hash table is most optimal. */
      ProvaTest *test = running_tests[i];
      if (test->child_pid == pid) {
        prova_collect_test(test, wstatus);
        stbds_arrdel(running_tests, i);
        running--;
        break;
      }
    }
  }
  stbds_arrfreef(running_tests);
}

int main(void) {
  prova_log_init(stdout);
  prova_launch_tests();

  size_t total = stbds_arrlenu(PROVA_TEST_QUEUE);
  size_t passed = 0, failed = 0, crashed = 0, skipped = 0;
  for (size_t i = 0; i < total; ++i) {
    switch (PROVA_TEST_QUEUE[i].status) {
    case TEST_PASS:
      passed++;
      break;
    case TEST_FAIL:
      failed++;
      break;
    case TEST_CRASH:
      crashed++;
      break;
    case TEST_SKIP:
      skipped++;
      break;
    default:
      break;
    }
  }
  prova_log("\nResults: %zu/%zu passed, %zu failed, %zu crashed, %zu skipped\n",
            passed, total, failed, crashed, skipped);
  prova_log_fini();
  return failed > 0 || crashed > 0;
}
