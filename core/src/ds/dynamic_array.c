/*
 * dynamic_array: simple resizeable array implementation
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "core/ds/dynamic_array.h"
#include "core/common.h"
#include "core/utils.h"

#include <stdlib.h>
#include <string.h>

void dynamic_array_init(dynamic_array *da, u64 size) {
  da->items = NULL;
  da->item_size = size;
  da->count = 0;
  da->capacity = 0;
}

void *dynamic_array_get_ptr(dynamic_array *da, u64 index) {
  if (!da || !da->items || index >= da->count) {
    scu_perror("Invalid Dynamic array passed to function.\n");
    return NULL;
  }

  return (char *)da->items + (index * da->item_size);
}

u32 dynamic_array_get(dynamic_array *da, u64 index, void *item) {
  if (!da || !item || index >= da->count || !da->items) {
    scu_perror("Invalid Dynamic array passed to function.\n");
    return 1;
  }

  void *src = (char *)da->items + (index * da->item_size);
  memcpy(item, src, da->item_size);
  return 0;
}

u32 dynamic_array_set(dynamic_array *da, u64 index, void *item) {
  if (!da || !item || !da->items || da->item_size == 0 || index >= da->count ||
      da->capacity == 0) {
    scu_perror("Invalid dynamic array passed to function.\n");
    return 1;
  }

  void *dest = (char *)da->items + (index * da->item_size);
  memcpy(dest, item, da->item_size);
  return 0;
}

u32 dynamic_array_append(dynamic_array *da, void *item) {
  if (!da || !item || da->item_size == 0) {
    scu_perror("Invalid dynamic array passed to function.\n");
    return 1;
  }

  if (da->capacity == 0) {
    da->capacity = 4;
    da->items = scu_checked_malloc(da->item_size * da->capacity);
    if (!da->items) {
      scu_perror("Failed to allocate dynamic array\n");
      return 1;
    }
  }

  if (da->count == da->capacity) {
    u64 new_capacity = da->capacity * 2;
    void *new_items =
        scu_checked_realloc(da->items, da->item_size * new_capacity);
    da->items = new_items;
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;
  return 0;
}

u32 dynamic_array_insert(dynamic_array *da, u64 index, void *item) {
  if (!da || !item || da->item_size == 0 || index > da->count) {
    scu_perror("Invalid dynamic array passed to function.\n");
    return 1;
  }

  if (da->count == da->capacity) {
    u64 new_capacity = da->capacity * 2;
    void *new_items =
        scu_checked_realloc(da->items, da->item_size * new_capacity);
    da->items = new_items;
    da->capacity = new_capacity;
  }

  memmove((char *)da->items + (index * da->item_size) + da->item_size,
          (char *)da->items + (index * da->item_size),
          ((da->count - index) * da->item_size));
  memcpy((char *)da->items + (index * da->item_size), item, da->item_size);
  da->count++;
  return 0;
}

u32 dynamic_array_remove(dynamic_array *da, u64 index) {
  if (!da || da->item_size == 0 || index >= da->count) {
    scu_perror("Invalid dynamic array passed to function.\n");
    return 1;
  }

  if (index != da->count - 1) {
    memmove((char *)da->items + (index * da->item_size),
            (char *)da->items + (index * da->item_size) + da->item_size,
            (da->count - index - 1) * da->item_size);
  }

  da->count--;
  return 0;
}

u32 dynamic_array_pop(dynamic_array *da, void *item) {
  if (!da || da->item_size == 0 || da->count == 0) {
    scu_perror("Invalid dynamic array or array is empty.\n");
    return 1;
  }

  if (item != NULL) {
    u32 return_value = dynamic_array_get(da, da->count - 1, item);
    if (return_value != 0) {
      return -1;
    }
  }

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
  for (u64 i = 0; i < da->count; i++) {
    char *item = (char *)da->items + (i * da->item_size);
    free(item);
  }
}
