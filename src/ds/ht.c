#include "ds/ht.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * @brief: check if x is prime.
 */
static int is_prime(const int x) {
  if (x < 2) {
    return -1;
  }
  if (x < 4) {
    return 1;
  }
  if ((x % 2) == 0) {
    return 0;
  }
  for (int i = 3; i <= floor(sqrt((double)x)); i += 2) {
    if ((x % i) == 0) {
      return 0;
    }
  }
  return 1;
}

/*
 * @brief: find the next prime number which is greater than x.
 */
static int next_prime(int x) {
  while (is_prime(x) != 1) {
    x++;
  }
  return x;
}

/*
 * @brief: a simple hash function.
 *
 * @param s: string to hash
 * @param prime: a prime number
 * @param m: size of the hash table
 */
static inline int ht_hash(const char *s, const int prime, const int m) {
  long hash = 0;
  const int len_s = strlen(s);
  long p_pow = 1;

  for (int i = 0; i < len_s; i++) {
    hash = (hash + s[i] * p_pow) % m;
    p_pow = (p_pow * prime) % m;
  }

  return (int)hash;
}

/*
 * @brief: implements double-hashing to avoid collissions.
 *
 * @param s: string to hash
 * @param num_buckets: size of the hash table
 * @param attempts: number of attempts it took to hash the current string
 */
static int ht_get_hash(const char *s, const int num_buckets,
                       const int attempt) {
#define HT_PRIME_1 0x21914047
#define HT_PRIME_2 0x1b873593
  const int hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
  const int hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
  return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
#undef HT_PRIME_1
#undef HT_PRIME_2
}

/*
 * @brief: allocate and initialize a new ht_item and return its memory address.
 *
 * @param k: key string literal.
 * @param v: pointer to the value to be stored.
 * @param value_size: number of bytes to be copied.
 */
static ht_item *ht_new_item(const char *k, const void *v,
                            const size_t value_size) {
  ht_item *i = malloc(sizeof(ht_item));
  i->key = strdup(k);
  i->value = malloc(value_size);
  memcpy(i->value, v, value_size);
  return i;
}

/*
 * @brief: free / delete an existing ht_item.
 *
 * @param i: pointer to the item to be freed.
 */
static void ht_del_item(ht_item *i) {
  free(i->key);
  free(i->value);
  free(i);
}

/*
 * @brief: represents a deleted or NULL hash table item.
 */
static ht_item HT_DELETED_ITEM = {NULL, NULL};

/*
 * @brief : create a new sized hash table
 *
 * @param base_capacity
 * @param value_size: number of bytes the value will occupy.
 */
static ht *ht_new_sized(const size_t base_capacity, const size_t value_size) {
  ht *table = malloc(sizeof(ht));

  table->base_capacity = base_capacity;
  table->capacity = next_prime(table->base_capacity);
  table->count = 0;
  table->items = calloc(table->capacity, sizeof(ht_item *));
  table->value_size = value_size;

  return table;
}

ht *ht_new(const size_t value_size) { return ht_new_sized(53, value_size); }

/*
 * @brief: resize an existing hash table to avoid high collission rates and keep
 * storing more key-value pairs.
 *
 * @param table: pointer to an initialized ht struct.
 * @param base_capacity
 */
static void ht_resize(ht *table, const size_t base_capacity) {
  if (base_capacity < 53) {
    return;
  }

  ht *new_ht = ht_new_sized(base_capacity, table->value_size);
  for (size_t i = 0; i < table->capacity; i++) {
    ht_item *item = table->items[i];
    if (item != NULL && item != &HT_DELETED_ITEM) {
      ht_insert(new_ht, item->key, item->value);
    }
  }

  table->base_capacity = new_ht->base_capacity;
  table->count = new_ht->count;

  const int tmp_size = table->capacity;
  table->capacity = new_ht->capacity;
  new_ht->capacity = tmp_size;

  ht_item **tmp_items = table->items;
  table->items = new_ht->items;
  new_ht->items = tmp_items;

  ht_del_ht(new_ht);
}

/*
 * @brief: wrapper function to make the hash table bigger.
 *
 * @param table: pointer to an initialized ht struct.
 */
static void ht_resize_up(ht *table) {
  const int new_size = table->base_capacity * 2;
  ht_resize(table, new_size);
}

/*
 * @brief: wrapper function to make the hash table smaller.
 *
 * @param table: pointer to an initialized ht struct.
 */
static void ht_resize_down(ht *table) {
  const int new_size = table->base_capacity / 2;
  ht_resize(table, new_size);
}

void ht_del_ht(ht *table) {
  if (table == NULL)
    return;

  if (table->items != NULL) {
    for (size_t i = 0; i < table->capacity; i++) {
      ht_item *item = table->items[i];
      if (item != NULL && item != &HT_DELETED_ITEM) { // Skip sentinel
        ht_del_item(item);
      }
    }
    free(table->items);
  }
  free(table);
}

void ht_insert(ht *table, const char *key, const void *value) {
  const int load = table->count * 100 / table->capacity;

  if (load > 70) {
    ht_resize_up(table);
  }

  ht_item *item = ht_new_item(key, value, table->value_size);
  int index = ht_get_hash(item->key, table->capacity, 0);
  ht_item *current = table->items[index];

  int i = 1;
  while (current != NULL) {
    if (current != &HT_DELETED_ITEM) {
      if (strcmp(current->key, key) == 0) {
        ht_del_item(current);
        table->items[index] = item;
        return;
      }
    }
    index = ht_get_hash(item->key, table->capacity, i);
    current = table->items[index];
    i++;
  }

  table->items[index] = item;
  table->count++;
}

void *ht_search(ht *table, const char *key) {
  int index = ht_get_hash(key, table->capacity, 0);
  ht_item *item = table->items[index];

  int i = 1;
  while (item != NULL) {
    if (item != &HT_DELETED_ITEM) {
      if (strcmp(item->key, key) == 0) {
        return item->value;
      }
    }
    index = ht_get_hash(key, table->capacity, i);
    item = table->items[index];
    i++;
  }

  return NULL;
}

void ht_delete(ht *table, const char *key) {
  const int load = table->count * 100 / table->capacity;

  if (load < 10) {
    ht_resize_down(table);
  }

  int index = ht_get_hash(key, table->capacity, 0);
  ht_item *item = table->items[index];

  int i = 0;
  while (item != NULL) {
    if (item != &HT_DELETED_ITEM) {
      if (strcmp(item->key, key) == 0) {
        ht_del_item(item);
        table->items[index] = &HT_DELETED_ITEM;
      }
    }
    index = ht_get_hash(key, table->capacity, i);
    item = table->items[index];
    i++;
  }

  table->count++;
}
