/*
 * stack: simple LIFO data structure implementation
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef STACK_H
#define STACK_H

#include "core/common.h"

typedef struct stack {
  void *items;
  u64 item_size;
  u64 count;
  u64 capacity;
} stack;

/*
 * @brief: Initializes an already allocated stack with the specified item size.
 *
 * @param s: pointer to a stack
 * @param item_size: size of each element in bytes
 */
void stack_init(stack *s, u64 item_size);

/*
 * @brief: Pushes an item onto the stack.
 *
 * @param s: pointer to a stack
 * @param item: pointer to the item to push
 */
void stack_push(stack *s, void *item);

/*
 * @brief: Pops and returns the top item from the stack.
 *
 * @param s: pointer to a stack
 * @param item: pointer to store the popped item
 */
void stack_pop(stack *s, void *item);

/*
 * @brief: Returns the top item without popping.
 *
 * @param s: pointer to a stack
 *
 * @return: pointer to top item, or NULL if stack is empty
 */
void *stack_top(stack *s);

/*
 * @brief: Frees the stack structure.
 *
 * @param s: pointer to a stack
 */
void stack_free(stack *s);

#endif // !STACK_H
