/*
 * dynamic_array: simple resizeable array implementation
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "core/common.h"

typedef struct dynamic_array {
  void *items;
  u64 item_size;
  u64 count;
  u64 capacity;
} dynamic_array;

/*
 * @brief: Initializes an already allocated dynamic array with the specified
 * item size.
 *
 * @param da: pointer to a dynamic_array
 * @param size: size of each element in bytes
 */
void dynamic_array_init(dynamic_array *da, u64 size);

/*
 * @brief: Gets  pointer to the item at the specified index.
 *
 * @param da: pointer to a dynamic_array
 * @param index: index of the item to get
 *
 * @return: pointer to item at specified index
 */
void *dynamic_array_get_ptr(dynamic_array *da, u64 index);

/*
 * @brief: Gets an item at the specified index.
 *
 * @param da: pointer to a dynamic_array
 * @param index: index of the item to get
 * @param item: pointer to store the retrieved item
 */
void dynamic_array_get(dynamic_array *da, u64 index, void *item);

/*
 * @brief: Sets an item at the specified index.
 *
 * @param da: pointer to a dynamic_array
 * @param index: index of the item to set
 * @param item: pointer to the item to store
 */
void dynamic_array_set(dynamic_array *da, u64 index, void *item);

/*
 * @brief: Pushes / appends an item to the end of the array.
 *
 * @param da: pointer to a dynamic_array
 * @param item: pointer to the item to append
 */
void dynamic_array_push(dynamic_array *da, void *item);

/*
 * @brief: Inserts an item at the specified index.
 *
 * @param da: pointer to a dynamic_array
 * @param index: index at which to insert the item
 * @param item: pointer to the item to insert
 */
void dynamic_array_insert(dynamic_array *da, u64 index, void *item);

/*
 * @brief: Removes an item at the specified index.
 *
 * @param da: pointer to a dynamic_array
 * @param index: index of the item to remove
 *
 * @return: 0 if successful, 1 if index out of bounds
 */
void dynamic_array_remove(dynamic_array *da, u64 index);

/*
 * @brief: Removes and returns the last item in the array.
 *
 * @param da: pointer to a dynamic_array
 * @param item: pointer to store the popped item (can be NULL)
 */
void dynamic_array_pop(dynamic_array *da, void *item);

/*
 * @brief: Frees the dynamic array structure.
 *
 * @param da: pointer to a dynamic_array
 */
void dynamic_array_free(dynamic_array *da);

/*
 * @brief: Frees the dynamic array structure and all items it contains.
 *
 * @param da: pointer to a dynamic_array
 */
void dynamic_array_free_items(dynamic_array *da);

#endif // !DYNAMIC_ARRAY_H
