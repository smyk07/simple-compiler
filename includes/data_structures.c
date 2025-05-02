#include "data_structures.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct allocator {
  uint8_t *start;
  uint8_t *prev;
  uint8_t *top;
  uint64_t size;
} allocator;

int string_slice_to_owned(string_slice *ss, char **str) {
  if (!ss || !ss->str || !str)
    return -1;

  *str = (char *)malloc(ss->len + 1);
  if (!*str)
    return -1;

  memcpy(*str, ss->str, ss->len);
  (*str)[ss->len] = '\0'; // Null-terminate the string

  return 0;
}

void dynamic_array_init_allocator(dynamic_array *da, unsigned int item_size,
                                  allocator *allocator) {
  da->allocator = allocator;
  da->items = NULL;
  da->item_size = item_size;
  da->count = 0;
  da->capacity = 0;
}

void dynamic_array_init(dynamic_array *da, unsigned int item_size) {
  dynamic_array_init_allocator(da, item_size, NULL);
}

int dynamic_array_get(dynamic_array *da, unsigned int index, void *item) {
  if (!da || !item || index >= da->count || !da->items) {
    return -1;
  }

  void *source = (uint8_t *)da->items + (index * da->item_size);
  memcpy(item, source, da->item_size);
  return 0;
}

int dynamic_array_append(dynamic_array *da, const void *item) {
  if (!da || !item || da->item_size == 0) {
    perror("Invalid dynamic_array passed to append");
    return 1;
  }

  if (da->count >= da->capacity) {
    unsigned int new_capacity = da->capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // default initial capacity
    }

    void *new_items = realloc(da->items, new_capacity * da->item_size);
    if (new_items == NULL) {
      perror("Failed to reallocate dynamic array");
      return 1;
    }

    da->items = new_items;
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;

  return 0;
}
