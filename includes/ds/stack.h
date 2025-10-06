#ifndef STACK
#define STACK

#include <stddef.h>

typedef struct stack {
  void *items;
  size_t item_size;
  size_t count;
  size_t capacity;
} stack;

#define STACK_INITIAL_CAPACITY 4
#define STACK_RESIZE_FACTOR 2

int stack_init(stack *s, size_t item_size);

int stack_push(stack *s, void *item);

int stack_pop(stack *s, void *item);

void *stack_top(stack *s);

void stack_free(stack *s);

#endif // !STACK
