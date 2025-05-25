#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int scu_read_file(char *path, char **buffer) {
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

void scu_perror(int *errors, char *__restrict __format, ...) {
  (*errors)++;
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "[ERROR] ");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_pwarning(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stderr, "[WARNING] ");
  vfprintf(stderr, __format, args);
  va_end(args);
}

void scu_pdebug(char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  fprintf(stdout, "[DEBUG] ");
  vfprintf(stdout, __format, args);
  va_end(args);
}

void scu_check_errors(int *errors) {
  if (*errors > 0) {
    scu_pwarning("%d error(s) found\n", *errors);
    exit(1);
  }
}

void scu_assemble(char *asm_file, char *output_file) {
  char command[512];
  snprintf(command, sizeof(command), "fasm %s %s", asm_file, output_file);
  int result = system(command);
  if (result != 0) {
    fprintf(stderr, "Assembly failed with code %d\n", result);
  }
}
