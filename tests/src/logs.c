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

#include "tests/logs.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

/* stb_sprintf header-only implementation */
#define STB_SPRINTF_DECORATE(name) stb_##name
#define STB_SPRINTF_IMPLEMENTATION
#include "tests/stb_sprintf.h"

/* Ring buffer for async log messages.
 * Producer (main/test threads): lock-free write to tail
 * Consumer (logger thread): reads from head
 */
#define PROVA_LOG_BUFSIZE (1024 * 64) /* 64KB ring buffer */
#define PROVA_LOG_MSGMAX 512          /* max bytes per message */

typedef struct {
  char buf[PROVA_LOG_BUFSIZE]; /* ring buffer */
  volatile size_t head;        /* consumer read pos */
  volatile size_t tail;        /* producer write pos */
} LogRing;

typedef struct {
  FILE *dst;
  int active;
  thrd_t tid;
  LogRing ring;
  mtx_t mx; /* protects shutdown */
  cnd_t cv; /* signals flush/shutdown */
} LogCtx;

ProvaLog prova_log_ctx = {NULL, 0};
static LogCtx log_ctx_priv;
static pid_t log_pid; /* Parent PID; after fork, children have different PID */

/* Logger thread main loop.
 * Consumes from ring buffer, writes to destination.
 * Exits when active=0 and ring empty.
 */
static int log_thread_main(void *arg) {
  LogCtx *ctx = (LogCtx *)arg;
  char msg[PROVA_LOG_MSGMAX + 1];

  while (ctx->active || ctx->ring.head != ctx->ring.tail) {
    size_t head = ctx->ring.head;
    size_t tail = ctx->ring.tail;

    if (head == tail) {
      /* Ring empty, wait for signal or timeout */
      mtx_lock(&ctx->mx);
      if (ctx->ring.head == ctx->ring.tail && ctx->active) {
        cnd_timedwait(&ctx->cv, &ctx->mx,
                      &(struct timespec){
                          .tv_sec = 0, .tv_nsec = 10000000}); /* 10ms timeout */
      }
      mtx_unlock(&ctx->mx);
      continue;
    }

    /* Extract message from ring. Messages are prefixed with length byte. */
    size_t msglen = (unsigned char)ctx->ring.buf[head];
    head = (head + 1) % PROVA_LOG_BUFSIZE;

    /* Ensure we don't read past end (wrap boundary) */
    if (msglen > 0 && msglen <= PROVA_LOG_MSGMAX) {
      size_t bytes_to_read = msglen;
      size_t end_space = PROVA_LOG_BUFSIZE - head;

      if (bytes_to_read <= end_space) {
        memcpy(msg, &ctx->ring.buf[head], bytes_to_read);
        head = (head + bytes_to_read) % PROVA_LOG_BUFSIZE;
      } else {
        /* Message wraps around ring buffer */
        memcpy(msg, &ctx->ring.buf[head], end_space);
        memcpy(&msg[end_space], ctx->ring.buf, bytes_to_read - end_space);
        head = bytes_to_read - end_space;
      }

      msg[bytes_to_read] = '\0';
      fputs(msg, ctx->dst);
      fflush(ctx->dst);
    }

    ctx->ring.head = head;
  }

  return 0;
}

/* Lock-free enqueue of formatted message to ring buffer.
 * Returns: bytes enqueued, 0 if buffer full.
 */
static size_t log_enqueue(const char *msg, size_t len) {
  LogCtx *ctx = &log_ctx_priv;
  if (!ctx->active || len == 0 || len > PROVA_LOG_MSGMAX)
    return 0;

  size_t tail = ctx->ring.tail;
  size_t need = len + 1; /* +1 for length prefix */
  size_t avail =
      (ctx->ring.head + PROVA_LOG_BUFSIZE - tail - 1) % PROVA_LOG_BUFSIZE;

  if (avail < need) {
    /* Buffer full, drop oldest messages */
    return 0;
  }

  /* Write length prefix */
  size_t new_tail = (tail + 1) % PROVA_LOG_BUFSIZE;
  ctx->ring.buf[tail] = (unsigned char)len;
  tail = new_tail;

  /* Write message (may wrap) */
  size_t end_space = PROVA_LOG_BUFSIZE - tail;
  if (len <= end_space) {
    memcpy(&ctx->ring.buf[tail], msg, len);
    tail = (tail + len) % PROVA_LOG_BUFSIZE;
  } else {
    memcpy(&ctx->ring.buf[tail], msg, end_space);
    memcpy(ctx->ring.buf, &msg[end_space], len - end_space);
    tail = len - end_space;
  }

  ctx->ring.tail = tail;

  /* Signal logger thread */
  mtx_lock(&ctx->mx);
  cnd_signal(&ctx->cv);
  mtx_unlock(&ctx->mx);

  return len;
}

void prova_log_init(FILE *out) {
  if (log_ctx_priv.active)
    return; /* Already initialized */

  log_pid = getpid();

  if (!out)
    out = stdout;

  log_ctx_priv.dst = out;
  log_ctx_priv.active = 1;
  log_ctx_priv.ring.head = 0;
  log_ctx_priv.ring.tail = 0;

  mtx_init(&log_ctx_priv.mx, mtx_plain);
  cnd_init(&log_ctx_priv.cv);

  /* Spawn logger thread */
  if (thrd_create(&log_ctx_priv.tid, log_thread_main, &log_ctx_priv) !=
      thrd_success) {
    log_ctx_priv.active = 0;
    fprintf(stderr, "prova_log_init: failed to create logger thread\n");
    return;
  }

  prova_log_ctx.dst = out;
  prova_log_ctx.active = 1;
}

void prova_log(const char *fmt, ...) {
  if (!log_ctx_priv.active || getpid() != log_pid)
    return;

  char buf[PROVA_LOG_MSGMAX];
  va_list args;
  va_start(args, fmt);
  int len = stb_vsnprintf(buf, PROVA_LOG_MSGMAX, fmt, args);
  va_end(args);

  if (len > 0 && len < PROVA_LOG_MSGMAX) {
    log_enqueue(buf, len);
  }
}

void prova_log_test(const char *name, int passed, int total, const char *msg) {
  if (!log_ctx_priv.active || getpid() != log_pid)
    return;

  char buf[PROVA_LOG_MSGMAX];
  const char *status = passed ? "PASS" : "FAIL";

  int len;
  if (msg) {
    len = stb_snprintf(buf, PROVA_LOG_MSGMAX, "[%s] %s: %s (%d/%d)\n", name,
                       status, msg, passed, total);
  } else {
    len = stb_snprintf(buf, PROVA_LOG_MSGMAX, "[%s] %s (%d/%d)\n", name, status,
                       passed, total);
  }

  if (len > 0 && len < PROVA_LOG_MSGMAX) {
    log_enqueue(buf, len);
  }
}

void prova_log_assert(const char *file, int line, const char *expr, int ok) {
  if (!log_ctx_priv.active || getpid() != log_pid)
    return;

  char buf[PROVA_LOG_MSGMAX];
  const char *status = ok ? "PASS" : "FAIL";

  int len = stb_snprintf(buf, PROVA_LOG_MSGMAX, "  %s:%d: %s [%s]\n", file,
                         line, expr, status);

  if (len > 0 && len < PROVA_LOG_MSGMAX) {
    log_enqueue(buf, len);
  }
}

void prova_log_fini(void) {
  if (!log_ctx_priv.active || getpid() != log_pid)
    return;

  /* Signal logger thread to finish */
  mtx_lock(&log_ctx_priv.mx);
  log_ctx_priv.active = 0;
  cnd_broadcast(&log_ctx_priv.cv); /* Wake up thread if waiting */
  mtx_unlock(&log_ctx_priv.mx);

  /* Wait for logger thread to drain ring buffer and exit */
  thrd_join(log_ctx_priv.tid, NULL);

  /* Cleanup */
  mtx_destroy(&log_ctx_priv.mx);
  cnd_destroy(&log_ctx_priv.cv);

  prova_log_ctx.active = 0;
}
