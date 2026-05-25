/*
 * stack: simple LIFO data structure implementation
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "core/ds/stack.h"
#include "core/common.h"
#include "core/utils.h"

#include <stdlib.h>
#include <string.h>

#define STACK_INITIAL_CAPACITY 4
#define STACK_RESIZE_FACTOR 2

void stack_init(stack *s, u64 item_size) {
  if (s == NULL)
    scu_punreachable("null stack");

  s->item_size = item_size;
  s->capacity = STACK_INITIAL_CAPACITY;
  s->items = scu_checked_malloc(s->item_size * s->capacity);
  s->count = 0;
}

static void stack_expand(stack *s) {
  if (s == NULL)
    scu_punreachable("null stack");
  if (!s->items || s->item_size == 0)
    scu_punreachable("uninitialized stack");

  u64 new_capacity = s->capacity * STACK_RESIZE_FACTOR;
  void *new_items = scu_checked_realloc(s->items, s->item_size * new_capacity);
  s->items = new_items;
  s->capacity = new_capacity;
}

static void stack_shrink(stack *s) {
  if (s == NULL)
    scu_punreachable("null stack");
  if (!s->items || s->item_size == 0)
    scu_punreachable("uninitialized stack");

  u64 new_capacity = s->capacity / STACK_RESIZE_FACTOR;
  if (new_capacity < STACK_INITIAL_CAPACITY) {
    new_capacity = STACK_INITIAL_CAPACITY;
  }
  void *new_items = scu_checked_realloc(s->items, new_capacity * s->item_size);
  s->items = new_items;
  s->capacity = new_capacity;
}

void stack_push(stack *s, void *item) {
  if (s == NULL)
    scu_punreachable("null stack");
  if (!s->items || s->item_size == 0)
    scu_punreachable("uninitialized stack");
  if (!item)
    scu_punreachable("invalid item pointer");

  if (s->count == s->capacity)
    stack_expand(s);

  memcpy((char *)s->items + (s->count * s->item_size), item, s->item_size);
  s->count++;
}

void stack_pop(stack *s, void *item) {
  if (s == NULL)
    scu_punreachable("null stack");
  if (!s->items || s->item_size == 0)
    scu_punreachable("uninitialized stack");
  if (!item)
    scu_punreachable("invalid item pointer");
  if (s->count == 0)
    scu_punreachable("empty stack");

  memcpy(item, (char *)s->items + ((s->count - 1) * s->item_size),
         s->item_size);
  s->count--;

  if (s->count < (s->capacity / STACK_RESIZE_FACTOR))
    stack_shrink(s);
}

void *stack_top(stack *s) {
  if (s == NULL)
    scu_punreachable("null stack");
  if (!s->items || s->item_size == 0)
    scu_punreachable("uninitialized stack");
  if (s->count == 0)
    scu_punreachable("empty stack");

  return (char *)s->items + ((s->count - 1) * s->item_size);
}

void stack_free(stack *s) {
  if (!s)
    return;
  free(s->items);
  s->items = NULL;
  s->item_size = 0;
  s->count = 0;
  s->capacity = 0;
}
