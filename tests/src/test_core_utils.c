#include "core/common.h"
#include "tests/prova.h"

#include "core/utils.h"

#include <stdlib.h>
#include <unistd.h>

PTEST(scu_checked_malloc_realloc) {
  u64 size = 128;
  u32 *ptr = scu_checked_malloc(size);
  PROVA_ASSERT_NOT_NULL(ptr);

  for (u32 i = 0; i < 10; i++) {
    ptr[i] = i * 10;
  }
  for (u32 i = 0; i < 10; i++) {
    PROVA_ASSERT_EQUAL_INT(i * 10, ptr[i]);
  }

  u64 new_size = 256;
  ptr = (u32 *)scu_checked_realloc(ptr, new_size);
  PROVA_ASSERT_NOT_NULL(ptr);

  for (u32 i = 0; i < 10; i++) {
    PROVA_ASSERT_EQUAL_INT(i * 10, ptr[i]);
  }
}

PTEST(scu_extract_name) {
  char *t1 = scu_extract_name("main.scl");
  PROVA_ASSERT_EQUAL_STRING("main", t1);
  free(t1);

  char *t2 = scu_extract_name("modules/core.test.scl");
  PROVA_ASSERT_EQUAL_STRING("modules/core.test", t2);
  free(t2);

  char *t3 = scu_extract_name("Makefile");
  PROVA_ASSERT_EQUAL_STRING("Makefile", t3);
  free(t3);
}

PTEST(scu_read_file) {
  char *filepath = "../tests/fixtures/dummy.txt";

  char *buff = NULL;
  char *expected_contents = "Hello tester\n";

  u32 bytes = scu_read_file(filepath, &buff);

  PROVA_ASSERT_EQUAL_INT(13, bytes);
  PROVA_ASSERT_EQUAL_STRING(expected_contents, buff);

  free(buff);
}

PTEST(scu_format_string) {
  char *result =
      scu_format_string("Error at line %d in file %s", 42, "lexer.c");

  PROVA_ASSERT_NOT_NULL(result);
  PROVA_ASSERT_EQUAL_STRING("Error at line 42 in file lexer.c", result);

  free(result);
}

void capture_stream_output(int stream_fd,
                           void (*print_func)(char *__restrict, ...), char *fmt,
                           char *arg, char *out_buffer, size_t buf_size) {
  int pipe_fds[2];
  if (pipe(pipe_fds) == -1) {
    perror("Failed to create pipe");
    exit(1);
  }

  int original_stream = dup(stream_fd);
  dup2(pipe_fds[1], stream_fd);

  print_func(fmt, arg);
  fflush(stream_fd == STDOUT_FILENO ? stdout : stderr);

  close(pipe_fds[1]);
  dup2(original_stream, stream_fd);
  close(original_stream);

  memset(out_buffer, 0, buf_size);
  read(pipe_fds[0], out_buffer, buf_size - 1);
  close(pipe_fds[0]);
}

PTEST(scu_print_functions) {
  char buffer[256];

  capture_stream_output(STDOUT_FILENO, (void *)scu_psuccess, "Success: %s\n",
                        "Loaded", buffer, sizeof(buffer));
  char *expected_success = "\033[1;32m[SUCCESS] \033[0mSuccess: Loaded\n";
  PROVA_ASSERT_EQUAL_STRING(expected_success, buffer);

  capture_stream_output(STDOUT_FILENO, (void *)scu_pdebug, "Debug: %s\n",
                        "Loaded", buffer, sizeof(buffer));
  char *expected_debug = "\033[1;32m[DEBUG] \033[0mDebug: Loaded\n";
  PROVA_ASSERT_EQUAL_STRING(expected_debug, buffer);

  capture_stream_output(STDERR_FILENO, (void *)scu_pwarning, "Warning: %s\n",
                        "Loaded", buffer, sizeof(buffer));
  char *expected_warning = "\033[1;33m[WARNING] \033[0mWarning: Loaded\n";
  PROVA_ASSERT_EQUAL_STRING(expected_warning, buffer);

  capture_stream_output(STDERR_FILENO, (void *)scu_perror, "Error: %s\n",
                        "Loaded", buffer, sizeof(buffer));
  char *expected_error = "\033[1;31m[ERROR] \033[0mError: Loaded\n";
  PROVA_ASSERT_EQUAL_STRING(expected_error, buffer);
}
