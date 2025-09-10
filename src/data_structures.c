#include "data_structures.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int string_slice_to_owned(string_slice *ss, char **str) {
  if (!ss || !ss->str || !str)
    return -1;

  *str = (char *)malloc(ss->len + 1);
  if (!*str)
    return -1;

  memcpy(*str, ss->str, ss->len);
  (*str)[ss->len] = '\0';

  return 0;
}

void dynamic_array_init(dynamic_array *da, size_t size) {
  da->items = NULL;
  da->item_size = size;
  da->count = 0;
  da->capacity = 0;
}

int dynamic_array_get(dynamic_array *da, size_t index, void *item) {
  if (!da || !item || index >= da->count || !da->items) {
    scu_perror(NULL, "Invalid Dynamic array passed to function.\n");
    return -1;
  }

  void *src = (char *)da->items + (index * da->item_size);
  memcpy(item, src, da->item_size);
  return 0;
}

int dynamic_array_set(dynamic_array *da, size_t index, void *item) {
  if (!da || !item || !da->items || da->item_size == 0 || index >= da->count ||
      da->capacity == 0) {
    scu_perror(NULL, "Invalid dynamic array passed to function.\n");
    return -1;
  }

  void *dest = (char *)da->items + (index * da->item_size);
  memcpy(dest, item, da->item_size);
  return 0;
}

int dynamic_array_append(dynamic_array *da, void *item) {
  if (!da || !item || da->item_size == 0) {
    scu_perror(NULL, "Invalid dynamic array passed to function.\n");
    return -1;
  }

  if (da->capacity == 0) {
    da->capacity = 4;
    da->items = malloc(da->item_size * da->capacity);
    if (!da->items) {
      scu_perror(NULL, "Failed to allocate dynamic array\n");
      return -1;
    }
  }

  if (da->count == da->capacity) {
    unsigned int new_capacity = da->capacity * 2;
    void *new_items = realloc(da->items, da->item_size * new_capacity);
    if (!new_items) {
      perror("Failed to resize dynamic array.\n");
      return -1;
    }
    da->items = new_items;
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;
  return 0;
}

int dynamic_array_insert(dynamic_array *da, size_t index, void *item) {
  if (!da || !item || da->item_size == 0 || index > da->count) {
    scu_perror(NULL, "Invalid dynamic array passed to function.\n");
    return -1;
  }

  if (da->count == da->capacity) {
    unsigned int new_capacity = da->capacity * 2;
    void *new_items = realloc(da->items, da->item_size * new_capacity);
    if (!new_items) {
      scu_perror(NULL, "Failed to resize dynamic array.\n");
      return -1;
    }
    da->items = new_items;
    da->capacity = new_capacity;
  }

  memmove((char *)da->items + (index * da->item_size),
          (char *)da->items + (index * da->item_size) + da->item_size,
          (da->count * da->item_size) - ((da->count - index) * da->item_size));
  memcpy((char *)da->items + (index * da->item_size), item, da->item_size);
  da->count++;
  return 0;
}

int dynamic_array_remove(dynamic_array *da, size_t index) {
  if (!da || da->item_size == 0 || index >= da->count) {
    scu_perror(NULL, "Invalid dynamic array passed to function.\n");
    return -1;
  }

  if (index != da->count - 1) {
    memmove((char *)da->items + (index * da->item_size),
            (char *)da->items + (index * da->item_size) + da->item_size,
            (da->count - index - 1) * da->item_size);
  }

  da->count--;
  return 0;
}

int dynamic_array_pop(dynamic_array *da, void *item) {
  if (!da || da->item_size == 0 || da->count == 0) {
    scu_perror(NULL, "Invalid dynamic array passed to function.\n");
    return -1;
  }

  int return_value = dynamic_array_get(da, da->count - 1, item);
  if (return_value != 0)
    return return_value;

  da->count--;
  return 0;
}

void dynamic_array_free(dynamic_array *da) {
  if (!da)
    return;
  free(da->items);
  da->items = NULL;
  da->count = 0;
  da->capacity = 0;
  da->item_size = 0;
}

void dynamic_array_free_items(dynamic_array *da) {
  for (unsigned int i = 0; i < da->count; i++) {
    char *item = (char *)da->items + (i * da->item_size);
    free(item);
  }
}
