/*
 * ht: contains the hash table struct and its interface.
 */

#ifndef HT_H
#define HT_H

#include <stddef.h>

/*
 * @struct ht_item: represents an individual item inside a hash table.
 */
typedef struct ht_item {
  char *key;
  void *value;
} ht_item;

/*
 * @struct ht: represents the hash table.
 */
typedef struct ht {
  size_t base_capacity;
  size_t capacity;
  size_t count;

  ht_item **items;
  size_t value_size;
} ht;

/*
 * @brief: initialize a new hash table.
 *
 * @param value_size: size of the value (void *) of each item.
 *
 * @return: pointer to the newly initialized hash table.
 */
ht *ht_new(const size_t value_size);

/*
 * @brief: delete an existing hash table.
 *
 * @param table: pointer to an initialized ht (hash table) struct.
 */
void ht_del_ht(ht *table);

/*
 * @brief: insert a key value pair into the hash table.
 *
 * @param table: pointer to an initialized ht (hash table) struct.
 * @param key: key string literal.
 * @param value: pointer to the value which is to be stored (will be copied).
 */
void ht_insert(ht *table, const char *key, const void *value);

/*
 * @brief: search for a key inside the hash table and retrieve the stored value.
 *
 * @param table: pointer to an initialized ht (hash table) struct.
 * @param key: key string literal
 */
void *ht_search(ht *table, const char *key);

/*
 * @brief: delete a key-value pair from the hash table.
 *
 * @param table: pointer to an initialized ht (hash table) struct.
 * @param key: key string literal.
 */
void ht_delete(ht *table, const char *key);

#endif // !HT_H
