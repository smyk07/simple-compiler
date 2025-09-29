#include "utils.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void *scu_checked_malloc(size_t size) {
  if (size == 0)
    size = 1;
  void *ptr = calloc(1, size);
  if (ptr == NULL) {
    scu_perror(NULL, "Memory allocation failed.");
    exit(1);
  }
  return ptr;
}

void *scu_checked_realloc(void *ptr, size_t size) {
  if (size == 0)
    size = 1;
  void *newptr = realloc(ptr, size);
  if (newptr == NULL) {
    scu_perror(NULL, "Memory re-allocation failed.");
    exit(1);
  }
  return newptr;
}

char *scu_extract_name(const char *filename) {
  const char *dot = strrchr(filename, '.');
  size_t len;

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

int scu_read_file(const char *path, char **buffer, unsigned int *error_count) {
  struct stat path_stat;
  stat(path, &path_stat);

  if (!S_ISREG(path_stat.st_mode)) {
    scu_perror(error_count, "Given path is not a valid file.\n");
    scu_check_errors(error_count);
  }

  int tmp_capacity = MAX_LEN;
  char *tmp = scu_checked_malloc(tmp_capacity * sizeof(char));

  int tmp_size = 0;

  FILE *f = fopen(path, "r");

  if (f == NULL) {
    perror("fopen failed");
    free(tmp);
    exit(1);
  }

  int size = 0;

  do {
    if (tmp_size + MAX_LEN >= tmp_capacity) {
      tmp_capacity *= 2;
      tmp = scu_checked_realloc(tmp, tmp_capacity * sizeof(char));
    }

    size = fread(tmp + tmp_size, sizeof(char), MAX_LEN, f);
    tmp_size += size;
  } while (size > 0);

  fclose(f);

  *buffer = tmp;

  return tmp_size;
}

#undef MAX_LEN

char *scu_format_string(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);

  va_list args_copy;
  va_copy(args_copy, args);
  int length = vsnprintf(NULL, 0, __format, args_copy);
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

void scu_perror(unsigned int *errors, char *__restrict __format, ...) {
  if (errors != NULL) {
    (*errors)++;
  }
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "\033[1;31m[ERROR] \033[0m");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_check_errors(unsigned int *errors) {
  if (*errors) {
    scu_pwarning("%d error(s) found\n", *errors);
    exit(1);
  }
}
