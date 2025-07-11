#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int scu_read_file(char *path, char **buffer, unsigned int *error_count) {
  struct stat path_stat;
  stat(path, &path_stat);
  if (!S_ISREG(path_stat.st_mode)) {
    scu_perror(error_count, "Given path is not a valid file.\n");
    scu_check_errors(error_count);
  }

  int tmp_capacity = MAX_LEN;
  char *tmp = malloc(tmp_capacity * sizeof(char));

  if (tmp == NULL) {
    perror("malloc failed");
    exit(1);
  }

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
      tmp = realloc(tmp, tmp_capacity * sizeof(char));

      if (tmp == NULL) {
        perror("realloc failed");
        fclose(f);
        exit(1);
      }
    }

    size = fread(tmp + tmp_size, sizeof(char), MAX_LEN, f);
    tmp_size += size;
  } while (size > 0);

  fclose(f);

  *buffer = tmp;

  return tmp_size;
}

char *scu_extract_name(const char *filename) {
  const char *dot = strrchr(filename, '.');
  size_t len;

  if (dot != NULL) {
    len = dot - filename;
  } else {
    len = strlen(filename);
  }

  char *name = (char *)malloc(len + 1);
  if (name == NULL) {
    return NULL;
  }

  strncpy(name, filename, len);
  name[len] = '\0';

  return name;
}

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

  char *result = malloc(length + 1);
  if (!result) {
    va_end(args);
    return NULL;
  }

  vsnprintf(result, length + 1, __format, args);
  va_end(args);

  return result;
}

void scu_psuccess(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stdout, "\033[1;32m");
  fprintf(stdout, "\t[SUCCESS] ");
  fprintf(stdout, "\033[0m");
  vfprintf(stdout, __format, args);
  va_end(args);
}

void scu_pdebug(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stdout, "\033[1;32m");
  fprintf(stdout, "\t[DEBUG] ");
  fprintf(stdout, "\033[0m");
  vfprintf(stdout, __format, args);
  va_end(args);
}

void scu_pwarning(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "\033[1;33m");
  fprintf(stderr, "\t[WARNING] ");
  fprintf(stderr, "\033[0m");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_perror(unsigned int *errors, char *__restrict __format, ...) {
  if (errors != NULL) {
    (*errors)++;
  }
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "\033[1;31m");
  fprintf(stderr, "\t[ERROR] ");
  fprintf(stderr, "\033[0m");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_check_errors(unsigned int *errors) {
  if (*errors) {
    scu_pwarning("%d error(s) found\n", *errors);
    exit(1);
  }
}

void scu_assemble(char *asm_file, char *output_file, unsigned int *errors) {
  char command[512];
  snprintf(command, sizeof(command), "fasm %s %s", asm_file, output_file);
  int result = system(command);
  if (result != 0) {
    scu_perror(errors, "Assembly failed with code %d\n", result);
    exit(1);
  }
}
