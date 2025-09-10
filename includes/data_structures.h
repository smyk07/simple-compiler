/*
 * file: data_structures.h
 * brief: contains data structures and functions related to them for the
 * simple-compiler.
 */
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

#include <stddef.h>

typedef struct string_slice {
  const char *str;
  size_t len;
} string_slice;

typedef struct dynamic_array {
  void *items;
  size_t item_size;
  size_t count;
  size_t capacity;
} dynamic_array;

int string_slice_to_owned(string_slice *ss, char **str);

void dynamic_array_init(dynamic_array *da, size_t size);

int dynamic_array_get(dynamic_array *da, size_t index, void *item);

int dynamic_array_set(dynamic_array *da, size_t index, void *item);

int dynamic_array_append(dynamic_array *da, void *item);

int dynamic_array_insert(dynamic_array *da, size_t index, void *item);

int dynamic_array_remove(dynamic_array *da, size_t index);

int dynamic_array_pop(dynamic_array *da, void *item);

void dynamic_array_free(dynamic_array *da);

void dynamic_array_free_items(dynamic_array *da);

#endif
