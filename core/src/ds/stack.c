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

u32 stack_init(stack *s, u64 item_size) {
  if (!s) {
    scu_perror("Invalid stack pointer passed to stack_init.\n");
    return 1;
  }
  s->item_size = item_size;
  s->capacity = STACK_INITIAL_CAPACITY;
  s->items = scu_checked_malloc(s->item_size * s->capacity);
  s->count = 0;
  return 0;
}

static u32 stack_expand(stack *s) {
  if (!s) {
    scu_perror("Invalid stack pointer passed to stack_expand.\n");
    return 1;
  }
  if (!s->items) {
    scu_perror("Uninitialized stack passed to stack_expand.\n");
    return 1;
  }
  u64 new_capacity = s->capacity * STACK_RESIZE_FACTOR;
  void *new_items = scu_checked_realloc(s->items, s->item_size * new_capacity);
  s->items = new_items;
  s->capacity = new_capacity;
  return 0;
}

static u32 stack_shrink(stack *s) {
  if (!s) {
    scu_perror("Invalid stack pointer passed to stack_shrink.\n");
    return 1;
  }
  if (!s->items) {
    scu_perror("Uninitialized stack passed to stack_shrink.\n");
    return 1;
  }
  u64 new_capacity = s->capacity / STACK_RESIZE_FACTOR;
  if (new_capacity < STACK_INITIAL_CAPACITY) {
    new_capacity = STACK_INITIAL_CAPACITY;
  }
  void *new_items = scu_checked_realloc(s->items, new_capacity * s->item_size);
  s->items = new_items;
  s->capacity = new_capacity;
  return 0;
}

u32 stack_push(stack *s, void *item) {
  if (!s || !item) {
    scu_perror("Invalid parameter passed to stack_push.\n");
    return 1;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror("Uninitialized stack passed to stack_push.\n");
    return 1;
  }

  if (s->count == s->capacity) {
    if (stack_expand(s) != 0) {
      scu_perror("Stack resize failed in stack_push.\n");
      return 1;
    }
  }

  memcpy((char *)s->items + (s->count * s->item_size), item, s->item_size);
  s->count++;
  return 0;
}

u32 stack_pop(stack *s, void *item) {
  if (!s || !item) {
    scu_perror("Invalid parameter passed to stack_pop.\n");
    return 1;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror("Uninitialized stack passed to stack_pop.\n");
    return 1;
  }

  if (s->count == 0)
    return 1;

  memcpy(item, (char *)s->items + ((s->count - 1) * s->item_size),
         s->item_size);
  s->count--;

  if (s->count < (s->capacity / STACK_RESIZE_FACTOR)) {
    if (stack_shrink(s) != 0) {
      scu_perror("Stack resize failed in stack_pop.\n");
      return 1;
    }
  }

  return 0;
}

void *stack_top(stack *s) {
  if (!s) {
    scu_perror("Invalid stack pointer passed to stack_top.\n");
    return NULL;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror("Uninitialized stack passed to stack_top.\n");
    return NULL;
  }

  if (s->count == 0) {
    scu_perror("Cannot get top of empty stack.\n");
    return NULL;
  }

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
