/*
 * file: data_structures.h
 * brief: contains data structures and functions related to them for the
 * simple-compiler.
 */
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

#include <stddef.h>

typedef struct allocator allocator;

typedef struct string_slice {
  char *str;
  unsigned int len;
} string_slice;

typedef struct dynamic_array {
  void *items;
  size_t item_size;
  unsigned int count;
  unsigned int capacity;
} dynamic_array;

int string_slice_to_owned(string_slice *ss, char **str);

void dynamic_array_init(dynamic_array *da, size_t size);
int dynamic_array_get(dynamic_array *da, unsigned int index, void *item);
int dynamic_array_set(dynamic_array *da, unsigned int index, void *item);
int dynamic_array_append(dynamic_array *da, void *item);
int dynamic_array_insert(dynamic_array *da, unsigned int index, void *item);
int dynamic_array_remove(dynamic_array *da, unsigned int index);
int dynamic_array_pop(dynamic_array *da, void *item);
void dynamic_array_free(dynamic_array *da);

#endif
