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

#define DA_CHECK_NULL(da)                                                      \
  if (!da)                                                                     \
  scu_punreachable("null dynamic_array")

#define DA_CHECK_INIT(da)                                                      \
  if (da->item_size == 0)                                                      \
  scu_punreachable("uninitialized dynamic_array")

#define DA_CHECK_NONEMPTY(da)                                                  \
  if (da->count == 0)                                                          \
  scu_punreachable("empty dynamic_array")

#define DA_CHECK_BOUNDS(da, index)                                             \
  if (index >= da->count)                                                      \
  scu_punreachable("index out of bounds")

#define DA_CHECK_ITEM(item)                                                    \
  if (!item)                                                                   \
  scu_punreachable("null item pointer")

void dynamic_array_init(dynamic_array *da, u64 size) {
  da->items = NULL;
  da->item_size = size;
  da->count = 0;
  da->capacity = 0;
}

void *dynamic_array_get_ptr(dynamic_array *da, u64 index) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_BOUNDS(da, index);

  return (char *)da->items + (index * da->item_size);
}

void dynamic_array_get(dynamic_array *da, u64 index, void *item) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_ITEM(item);
  DA_CHECK_BOUNDS(da, index);

  void *src = (char *)da->items + (index * da->item_size);
  memcpy(item, src, da->item_size);
}

void dynamic_array_set(dynamic_array *da, u64 index, void *item) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_ITEM(item);
  DA_CHECK_BOUNDS(da, index);

  void *dest = (char *)da->items + (index * da->item_size);
  memcpy(dest, item, da->item_size);
}

void dynamic_array_append(dynamic_array *da, void *item) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_ITEM(item);

  if (da->capacity == 0) {
    da->capacity = 4;
    da->items = scu_checked_malloc(da->item_size * da->capacity);
  }

  if (da->count == da->capacity) {
    u64 new_capacity = da->capacity * 2;
    da->items = scu_checked_realloc(da->items, da->item_size * new_capacity);
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;
}

void dynamic_array_insert(dynamic_array *da, u64 index, void *item) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_ITEM(item);
  DA_CHECK_BOUNDS(da, index);

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
}

void dynamic_array_remove(dynamic_array *da, u64 index) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_BOUNDS(da, index);

  if (index != da->count - 1) {
    memmove((char *)da->items + (index * da->item_size),
            (char *)da->items + (index * da->item_size) + da->item_size,
            (da->count - index - 1) * da->item_size);
  }

  da->count--;
}

void dynamic_array_pop(dynamic_array *da, void *item) {
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);
  DA_CHECK_ITEM(item);
  DA_CHECK_NONEMPTY(da);

  dynamic_array_get(da, da->count - 1, item);
  da->count--;
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
  DA_CHECK_NULL(da);
  DA_CHECK_INIT(da);

  for (u64 i = 0; i < da->count; i++) {
    char *item = (char *)da->items + (i * da->item_size);
    free(item);
  }
}
