/*
 * file: data_structures.h
 * brief: contains data structures and functions related to them for the
 * simple-compiler.
 */
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

typedef struct allocator allocator;

typedef struct string_slice {
  struct ds_allocator *allocator;
  char *str;
  unsigned int len;
} string_slice;

typedef struct dynamic_array {
  allocator *allocator;
  void *items;
  unsigned int item_size;
  unsigned int count;
  unsigned int capacity;
} dynamic_array;

int string_slice_to_owned(string_slice *ss, char **str);

void dynamic_array_init(dynamic_array *da, unsigned int item_size);
int dynamic_array_get(dynamic_array *da, unsigned int index, void *item);
int dynamic_array_append(dynamic_array *da, const void *item);
void dynamic_array_free(dynamic_array *da);

#endif
