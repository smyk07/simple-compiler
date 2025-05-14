#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_file(char *path, char **buffer) {
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

char *extract_name(const char *filename) {
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

void assemble(char *asm_file, char *output_file) {
  char command[512];
  snprintf(command, sizeof(command), "fasm %s %s", asm_file, output_file);
  int result = system(command);
  if (result != 0) {
    fprintf(stderr, "Assembly failed with code %d\n", result);
  }
}
