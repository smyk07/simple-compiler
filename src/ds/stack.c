#include "ds/stack.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

int stack_init(stack *s, size_t item_size) {
  if (!s) {
    scu_perror(NULL, "Invalid stack pointer passed to stack_init.\n");
    return -1;
  }
  s->item_size = item_size;
  s->capacity = STACK_INITIAL_CAPACITY;
  s->items = malloc(s->item_size * s->capacity);
  if (!s->items) {
    s->capacity = 0;
    scu_perror(NULL, "Memory allocation failed in stack_init.\n");
    return -1;
  }
  s->count = 0;
  return 0;
}

static int stack_expand(stack *s) {
  if (!s) {
    scu_perror(NULL, "Invalid stack pointer passed to stack_expand.\n");
    return -1;
  }
  if (!s->items) {
    scu_perror(NULL, "Uninitialized stack passed to stack_expand.\n");
    return -1;
  }
  size_t new_capacity = s->capacity * STACK_RESIZE_FACTOR;
  void *new_items = realloc(s->items, s->item_size * new_capacity);
  if (!new_items) {
    scu_perror(NULL, "Memory allocation failed in stack_expand.\n");
    return -1;
  }
  s->items = new_items;
  s->capacity = new_capacity;
  return 0;
}

static int stack_shrink(stack *s) {
  if (!s) {
    scu_perror(NULL, "Invalid stack pointer passed to stack_shrink.\n");
    return -1;
  }
  if (!s->items) {
    scu_perror(NULL, "Uninitialized stack passed to stack_shrink.\n");
    return -1;
  }
  size_t new_capacity = s->capacity / STACK_RESIZE_FACTOR;
  if (new_capacity < STACK_INITIAL_CAPACITY) {
    new_capacity = STACK_INITIAL_CAPACITY;
  }
  void *new_items = realloc(s->items, new_capacity * s->item_size);
  if (new_items == NULL) {
    scu_perror(NULL, "Memory allocation failed in stack_shrink.\n");
    return -1;
  }
  s->items = new_items;
  s->capacity = new_capacity;
  return 0;
}

int stack_push(stack *s, void *item) {
  if (!s || !item) {
    scu_perror(NULL, "Invalid parameter passed to stack_push.\n");
    return -1;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror(NULL, "Uninitialized stack passed to stack_push.\n");
    return -1;
  }

  if (s->count == s->capacity) {
    if (stack_expand(s) != 0) {
      scu_perror(NULL, "Stack resize failed in stack_push.\n");
      return -1;
    }
  }

  memcpy((char *)s->items + (s->count * s->item_size), item, s->item_size);
  s->count++;
  return 0;
}

int stack_pop(stack *s, void *item) {
  if (!s || !item) {
    scu_perror(NULL, "Invalid parameter passed to stack_pop.\n");
    return -1;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror(NULL, "Uninitialized stack passed to stack_pop.\n");
    return -1;
  }

  if (s->count == 0) {
    scu_perror(NULL, "Cannot pop from empty stack.\n");
    return -1;
  }

  memcpy(item, (char *)s->items + ((s->count - 1) * s->item_size),
         s->item_size);
  s->count--;

  if (s->count < (s->capacity / STACK_RESIZE_FACTOR)) {
    if (stack_shrink(s) != 0) {
      scu_perror(NULL, "Stack resize failed in stack_pop.\n");
      return -1;
    }
  }

  return 0;
}

void *stack_top(stack *s) {
  if (!s) {
    scu_perror(NULL, "Invalid stack pointer passed to stack_top.\n");
    return NULL;
  }

  if (!s->items || s->item_size == 0) {
    scu_perror(NULL, "Uninitialized stack passed to stack_top.\n");
    return NULL;
  }

  if (s->count == 0) {
    scu_perror(NULL, "Cannot get top of empty stack.\n");
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
