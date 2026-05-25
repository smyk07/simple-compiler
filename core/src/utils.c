/*
 * utils: basic utility functions for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "core/utils.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void *scu_checked_malloc(u64 size) {
  if (size == 0)
    size = 1;
  void *ptr = calloc(1, size);
  if (ptr == NULL) {
    scu_perror("Memory allocation failed.");
    abort();
  }
  return ptr;
}

void *scu_checked_realloc(void *ptr, u64 size) {
  if (size == 0)
    size = 1;
  void *newptr = realloc(ptr, size);
  if (newptr == NULL) {
    scu_perror("Memory re-allocation failed.");
    abort();
  }
  return newptr;
}

char *scu_extract_name(const char *filename) {
  const char *dot = strrchr(filename, '.');
  u64 len;

  if (dot != NULL) {
    len = dot - filename;
  } else {
    len = strlen(filename);
  }

  char *name = (char *)scu_checked_malloc(len + 1);
  strncpy(name, filename, len);
  name[len] = '\0';

  return name;
}

#define MAX_LEN 4096

scu_result scu_read_file(const char *path, char **buffer, u64 *out_size) {
  struct stat path_stat;
  stat(path, &path_stat);

  if (stat(path, &path_stat) != 0) {
    scu_perror("Cannot access file: %s — %s\n", path, strerror(errno));
    return SCU_ERR_IO;
  }

  if (!S_ISREG(path_stat.st_mode)) {
    scu_perror("Not a regular file: %s\n", path);
    return SCU_ERR_ARGS;
  }

  u32 tmp_capacity = MAX_LEN;
  char *tmp = scu_checked_malloc(tmp_capacity * sizeof(char));

  FILE *f = fopen(path, "r");

  if (f == NULL) {
    scu_perror("Cannot open file: %s — %s\n", path, strerror(errno));
    free(tmp);
    return SCU_ERR_IO;
  }

  u64 tmp_size = 0;
  u64 size = 0;

  do {
    if (tmp_size + MAX_LEN >= tmp_capacity) {
      tmp_capacity *= 2;
      tmp = scu_checked_realloc(tmp, tmp_capacity * sizeof(char));
    }

    size = fread(tmp + tmp_size, sizeof(char), MAX_LEN, f);
    tmp_size += size;
  } while (size > 0);

  if (ferror(f)) {
    scu_perror("Read error on file: %s\n", path);
    free(tmp);
    fclose(f);
    return SCU_ERR_IO;
  }

  fclose(f);

  *buffer = tmp;
  *out_size = tmp_size;

  return SCU_SUCCESS;
}

#undef MAX_LEN

char *scu_format_string(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);

  va_list args_copy;
  va_copy(args_copy, args);
  u32 length = vsnprintf(NULL, 0, __format, args_copy);
  va_end(args_copy);

  if (length < 0) {
    va_end(args);
    return NULL;
  }

  char *result = scu_checked_malloc(length + 1);

  vsnprintf(result, length + 1, __format, args);
  va_end(args);

  return result;
}

void scu_psuccess(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stdout, "\033[1;32m[SUCCESS] \033[0m");
  vfprintf(stdout, __format, args);
  va_end(args);
}

void scu_pdebug(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stdout, "\033[1;32m[DEBUG] \033[0m");
  vfprintf(stdout, __format, args);
  va_end(args);
}

void scu_pwarning(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "\033[1;33m[WARNING] \033[0m");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_perror(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "\033[1;31m[ERROR] \033[0m");
  vfprintf(stderr, __format, args);
  va_end(args);
}
