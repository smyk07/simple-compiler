/*
 * prova: Unit testing framework in C, for C.
 *
 * Copyright 2026 Abhigyan Kumar Abhigyan Kumar <314abh at gmail dot com>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

#include "tests/prova.h"

/* global structs */
PMeta p_metadata = {0};
PTest *p_registry = NULL;
thread_local PAssertCtx *p_assert_ctx = NULL;

/* TODO: implement cleaner method to report errors related to pipes and forks.
 */
void prova_run_tests(PTest *registry)
{
    PTest *curr = registry;
    size_t total_tests = prova_count_tests(curr);
    p_metadata.total_tests = total_tests;

    while (curr)
    {
        int pipefd[2];
        assert((pipe(pipefd) != -1) && "pipe: couldn't create pipe for new child process.");

        pid_t pid = fork();
        assert((pid >= 0) && "fork: couldn't create child process.");

        if (pid == 0)
        {
            close(pipefd[0]);

            /* execute test function within local context. assume passing by default. */
            PAssertCtx local_ctx = {TEST_PASS, 0, {0}};
            p_assert_ctx = &local_ctx;
            curr->function();

            write(pipefd[1], p_assert_ctx, sizeof(*p_assert_ctx));
            close(pipefd[1]);
            _exit(local_ctx.status == TEST_PASS ? 0 : 1);
        }
        else
        {
            close(pipefd[1]);
            // PStatus test_result;
            PAssertCtx local_ctx = { 0 };
            ssize_t n = read(pipefd[0], &local_ctx, sizeof(local_ctx));
            close(pipefd[0]);

            int wstatus;
            waitpid(pid, &wstatus, 0);
            if (n == sizeof(local_ctx) && local_ctx.status == TEST_PASS)
            {
                curr->status = TEST_PASS;
                p_metadata.passing_tests++;
            }
            else if (WIFSIGNALED(wstatus))
            {
                curr->status = TEST_CRASH;
                p_metadata.crashing_tests++;
            }
            else
            {
                curr->status = TEST_FAIL;
                p_metadata.failing_tests++;
                char *s = (char *) malloc(PROVA_FAIL_MSG_MAX);
                strncpy(s, local_ctx.fail_msg, PROVA_FAIL_MSG_MAX);
                curr->msg = s;
            }
        }

        curr = curr->next;
    }
}

size_t prova_count_tests(const PTest *registry)
{
    const PTest *curr = registry;
    size_t total = 0;
    while (curr)
    {
        total++;
        curr = curr->next;
    }

    return total;
}

void prova_print_summary(const PTest *registry)
{
    if (registry == NULL)
    {
        fprintf(stderr, "prova: no test cases provided.");
        return;
    }

    const PTest *curr = registry;
    while (curr)
    {
        switch (curr->status)
        {
        case TEST_FAIL:
            fprintf(stderr, "[FAILED] unit %s : %s\n", curr->name, curr->msg);
            break;
        case TEST_CRASH:
            fprintf(stderr, "[CRASHED] unit %s : %s\n", curr->name, curr->msg);
            break;
        case TEST_PENDING:
            fprintf(stderr, "[PENDING] unit %s : %s\n", curr->name, curr->msg);
            break;
        case TEST_SKIP:
            fprintf(stderr, "[SKIPPED] unit %s : %s\n", curr->name, curr->msg);
            break;
        default:
            break;
        }
        curr = curr->next;
    }

    printf("\n=== SUMMARY ===\n");
    printf("Total testcases: %u\n", p_metadata.total_tests);
    printf("Failing testcases: %u\n", p_metadata.failing_tests);
    printf("Skipped testcases: %u\n", p_metadata.skipping_tests);
    printf("Crashing testcases: %u\n", p_metadata.crashing_tests);
    printf("Passing ratio: %u/%u\n", p_metadata.passing_tests, p_metadata.total_tests);
}

void prova_cleanup_messages(const PTest *registry) {
    const PTest *curr = registry;
    while (curr) {
        PTest *next = curr->next;
        free(curr->msg);
        curr = next;
    }
}

int main(void)
{
    prova_run_tests(p_registry);
    prova_print_summary(p_registry);
    return (p_metadata.failing_tests + p_metadata.crashing_tests) > 0 ? 1 : 0;
}
